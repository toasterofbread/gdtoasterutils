#include "ds4godot.h"

#include <godot_cpp/core/class_db.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libinput.h>

using namespace godot;

static int open_restricted(const char *path, int flags, void *user_data){
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data){
    close(fd);
}

const static struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

void DS4Godot::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open", "device_path"), &DS4Godot::openDevice);
    ClassDB::bind_method(D_METHOD("close"), &DS4Godot::closeDevice);
    ClassDB::bind_method(D_METHOD("getOpenDeviceFd"), &DS4Godot::getOpenDeviceFd);
    ClassDB::bind_method(D_METHOD("grab"), &DS4Godot::grab);
    ClassDB::bind_method(D_METHOD("ungrab"), &DS4Godot::ungrab);
    ClassDB::bind_method(D_METHOD("getTouchpadFingerPosition"), &DS4Godot::getTouchpadFingerPosition);
    ClassDB::bind_method(D_METHOD("getDeviceList"), &DS4Godot::getDeviceList);
    ClassDB::bind_method(D_METHOD("getResolution"), &DS4Godot::getResolution);

    ADD_SIGNAL(MethodInfo("finger_position_changed", PropertyInfo(Variant::INT, "finger"), PropertyInfo(Variant::VECTOR2, "position"), PropertyInfo(Variant::VECTOR2, "relative")));
    ADD_SIGNAL(MethodInfo("finger_touching_changed", PropertyInfo(Variant::INT, "finger"), PropertyInfo(Variant::BOOL, "touching")));
    ADD_SIGNAL(MethodInfo("pad_pressed_changed", PropertyInfo(Variant::BOOL, "pressed")));
}

DS4Godot::~DS4Godot() {
    closeDevice();
}

void DS4Godot::_process(double delta) {
    if (device < 0) {
        return;
    }

    int ax = -1;
    int ay = -1;
    int bx = -1;
    int by = -1;

    int a_touching = -1;
    int b_touching = -1;
    int pressed = -1;

    int next_finger = -1;
    bool b_specified = false;
    bool abs_flag = false;

    input_event ev;
    int n = 0;

    while (true) {
        int size = read(device, &ev, sizeof(ev));
        if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                ERR_FAIL_MSG(vformat("Reading device %f failed (%s)", device, strerror(errno)));
                return;
            }
        }
        if (size != sizeof(ev)) {
            continue;
        }

        if (ev.type == EV_KEY) {
            if (ev.code == 330) {
                a_touching = ev.value;
            }
            else if (ev.code == 333) {
                b_touching = ev.value;
            }
            else if (ev.code == 272) {
                pressed = ev.value;
            }
            continue;
        }
        else if (ev.type != EV_ABS) {
            continue;
        }

        n++;

        if (ev.code == ABS_MT_SLOT) {
            next_finger = ev.value;
            if (next_finger == 1) {
                b_specified = true;
            }
            continue;
        }

        switch (ev.code) {
            case ABS_MT_POSITION_X:
                if (next_finger == 0)
                    ax = ev.value;
                else
                    bx = ev.value;
                break;
            case ABS_MT_POSITION_Y:
                if (next_finger == 0)
                    ay = ev.value;
                else
                    by = ev.value;
                break;

            case ABS_X:
            case ABS_Y:
                abs_flag = true;
                break;
        }
    }

    if (abs_flag && next_finger == -1) {
        ax = bx;
        ay = by;
        bx = -1;
        by = -1;
    }

    if (a_touching != -1 && a_touching != last_a_touching) {
        last_a_touching = a_touching;
        emit_signal("finger_touching_changed", 0, a_touching == 1);
    }
    if (b_touching != -1 && b_touching != last_b_touching) {
        last_b_touching = b_touching;
        emit_signal("finger_touching_changed", 1, b_touching == 1);
    }
    if (pressed != -1 && pressed != last_pressed) {
        last_pressed = pressed;
        emit_signal("pad_pressed_changed", pressed == 1);
    }

    bool a_updated = false;
    if (last_a_touching && (ax != -1 && ax != last_ax) && (ay != -1 && ay != last_ay)) {
        Vector2 relative = Vector2(
            ax == -1 ? 0 : ax - last_ax,
            ay == -1 ? 0 : ay - last_ay
        );

        if (ax != -1) {
            last_ax = ax;
        }
        if (ay != -1) {
            last_ay = ay;
        }

        emit_signal("finger_position_changed", 0, Vector2(last_ax, last_ay), relative);
        a_updated = true;
    }

    if (next_finger != -1 && !b_specified) {
        return;
    }

    if (last_b_touching && ((bx != -1 && bx != last_bx) && (by != -1 && by != last_by))) {
        Vector2 relative = Vector2(
            bx == -1 ? 0 : bx - last_bx,
            by == -1 ? 0 : by - last_by
        );

        if (bx != -1) {
            last_bx = bx;
        }
        if (by != -1) {
            last_by = by;
        }

        emit_signal("finger_position_changed", 1, Vector2(last_bx, last_by), relative);
        printf("%d %d %d %d %d %b %b\n", ax, ay, bx, by, next_finger, last_a_touching, last_b_touching);
    }
}

Vector2 DS4Godot::getResolution() {
    return Vector2(1920, 943);
}

int DS4Godot::openDevice(const String &p_dev_path) {
    if (device >= 0) {
        close(device);
    }

    device = open(p_dev_path.utf8().get_data(), O_RDONLY | O_NONBLOCK);
    if (device < 0) {
        return errno;
    }

    return 0;
}

void DS4Godot::closeDevice() {
    if (device >= 0) {
        close(device);
        device = -1;
    }
}

int DS4Godot::getOpenDeviceFd() {
    return device;
}

bool DS4Godot::grab() {
    if (device < 0) {
        return false;
    }
    return ioctl(device, EVIOCGRAB, 1) == 0;
}

bool DS4Godot::ungrab() {
    if (device < 0) {
        return false;
    }
    return ioctl(device, EVIOCGRAB, 0) == 0;
}

Vector2 DS4Godot::getTouchpadFingerPosition() {
    ERR_FAIL_COND_V_MSG(device < 0, Vector2(0, 0), vformat("No device open"));

    struct input_absinfo abs_x;
    if (ioctl(device, EVIOCGABS(ABS_X), &abs_x) < 0) {
        close(device);
        ERR_FAIL_V_MSG(Vector2(0, 0), vformat("Failed to get X axis of device %f (%s)", device, strerror(errno)));
    }

    struct input_absinfo abs_y;
    if (ioctl(device, EVIOCGABS(ABS_Y), &abs_y) < 0) {
        close(device);
        ERR_FAIL_V_MSG(Vector2(0, 0), vformat("Failed to get Y axis of device %f (%s)", device, strerror(errno)));
    }

    return Vector2(abs_x.value, abs_y.value);
}

Dictionary DS4Godot::getDeviceList() {
    struct udev *udev = udev_new();
    struct libinput *li = libinput_udev_create_context(&interface, NULL, udev);

    libinput_udev_assign_seat(li, "seat0");
    libinput_dispatch(li);

    Dictionary ret;

    struct libinput_event *ev;
    while ((ev = libinput_get_event(li))) {

        if (libinput_event_get_type(ev) == LIBINPUT_EVENT_DEVICE_ADDED){
            double w = 0, h = 0;
            struct libinput_device *dev = libinput_event_get_device(ev);

            if(libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_POINTER) && libinput_device_get_size(dev, &w, &h) == 0) {
                char path[20];
                snprintf(path, sizeof(path), "/dev/input/%s", libinput_device_get_sysname(dev));

                const char* name = libinput_device_get_name(dev);
                ret[path] = name;
            }
        }

        libinput_event_destroy(ev);
        libinput_dispatch(li);
    }

    libinput_unref(li);

    return ret;
}
