#include "kanjimatcher.h"

#include <vector>
#include <algorithm>
#include <zinnia.h>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void KanjiMatcher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("loadModel", "model_path"), &KanjiMatcher::loadModel);
    ClassDB::bind_method(D_METHOD("setSize", "size"), &KanjiMatcher::setSize);
    ClassDB::bind_method(D_METHOD("addStrokePoint", "stroke", "point"), &KanjiMatcher::addStrokePoint);
    ClassDB::bind_method(D_METHOD("clear"), &KanjiMatcher::clear);
    ClassDB::bind_method(D_METHOD("matchStrokes", "max_amount"), &KanjiMatcher::matchStrokes);
    ClassDB::bind_static_method("KanjiMatcher", D_METHOD("orderIndependentStrokeMatch", "model_path", "max_amount_per_match", "size", "strokes"), &KanjiMatcher::orderIndependentStrokeMatch);
}

KanjiMatcher::KanjiMatcher() {
    character = zinnia::Character::create();
    character->clear();
}

KanjiMatcher::~KanjiMatcher() {
    delete recognizer;
    delete character;
}

// Zinnia models are available at https://tegaki.github.io/
void KanjiMatcher::loadModel(const String &model_path) {
    if (recognizer != nullptr) {
        return;
    }

    recognizer = zinnia::Recognizer::create();

    if (!recognizer->open(model_path.utf8().get_data())) {
        const String msg = vformat("Opening model at %s failed. %s", model_path, recognizer->what());

        delete recognizer;
        recognizer = nullptr;

        ERR_FAIL_MSG(vformat("Opening model at %s failed. %s", model_path, msg));
    }
}

void KanjiMatcher::clear() {
    character->clear();
    size_set = false;
}

void KanjiMatcher::setSize(Vector2 size) {
    character->set_width(size.x);
    character->set_height(size.y);
    size_set = true;
}

void KanjiMatcher::addStrokePoint(int stroke, Vector2 point) {
    character->add(stroke, point.x, point.y);
}

Array KanjiMatcher::matchStrokes(int max_amount) {
    Array ret;

    ERR_FAIL_COND_V_MSG(!size_set, ret, "Size not set");
    ERR_FAIL_COND_V_MSG(recognizer == nullptr, ret, "Model not loaded");

    zinnia::Result *result = recognizer->classify(*character, max_amount);
    ERR_FAIL_COND_V_MSG(!result, ret, vformat("Classifying strokes failed. %s", recognizer->what()));

    for (size_t i = 0; i < result->size(); ++i) {
        Array item;
        item.append(String::utf8(result->value(i)));
        item.append(result->score(i) + 1.0);
        ret.append(item);
    }

    delete result;

    return ret;
}

Array KanjiMatcher::orderIndependentStrokeMatch(const String &model_path, int max_amount_per_match, Vector2 size, const Array strokes) {
    Array ret;

    std::vector<Variant> vec;
    vec.reserve(strokes.size());

    for (int i = 0; i < strokes.size(); i++) {
        Variant value = strokes[i];

        ERR_FAIL_COND_V_MSG(value.get_type() != Variant::ARRAY, ret, "Invalid stroke data");

        Array array = (Array)value;
        for (int j = 0; j < array.size(); j++) {
            ERR_FAIL_COND_V_MSG(array[j].get_type() != Variant::VECTOR2, ret, "Invalid stroke data");
        }

        vec.push_back(value);
    }

    KanjiMatcher matcher;
    matcher.loadModel(model_path);

    do {
        matcher.clear();
        matcher.setSize(size);
        for (int i = 0; i < vec.size(); i++) {
            Array points = vec[i];

            for (int j = 0; j < points.size(); j++) {
                matcher.addStrokePoint(i, points[j]);
            }
        }

        Array matches = matcher.matchStrokes(max_amount_per_match);

        for (int i = 0; i < matches.size(); i++) {
            Array item = matches[i];

            String key = item[0];
            Variant new_value = item[1];

            bool found = false;
            for (int j = 0; j < ret.size(); j++) {
                Array ret_item = ret[j];

                if (ret_item[0] == key) {
                    Variant& old_value = ret_item[1];
                    if (old_value < new_value) {
                        old_value = new_value;
                    }
                    found = true;
                    break;
                }
            }

            if (!found) {
                ret.append(item);
            }
        }

    } while (std::next_permutation(vec.begin(), vec.end()));

    return ret;
}
