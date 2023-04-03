#ifndef DS4GODOT_H
#define DS4GODOT_H

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class DS4Godot: public Node {
        GDCLASS(DS4Godot, Node)

        private:
            int device = -1;

            int last_ax;
            int last_ay;
            int last_bx;
            int last_by;

            bool last_a_touching = false;
            bool last_b_touching = false;
            bool last_pressed = false;

        protected:
            static void _bind_methods();

        public:
            ~DS4Godot();

            void _process(double delta) override;

            int openDevice(const String &p_dev_path);
            void closeDevice();
            int getOpenDeviceFd();

            bool grab();
            bool ungrab();

            Vector2 getTouchpadFingerPosition();
    };
}

#endif