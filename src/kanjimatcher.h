#ifndef KANJIMATCHER_H
#define KANJIMATCHER_H

#include <godot_cpp/core/binder_common.hpp>
#include <zinnia.h>

namespace godot {
    class KanjiMatcher: public Object {
        GDCLASS(KanjiMatcher, Object)

        private:
            zinnia::Recognizer* recognizer = nullptr;
            zinnia::Character* character;
            bool size_set = false;

        protected:
            static void _bind_methods();

        public:
            KanjiMatcher();
            ~KanjiMatcher();

            void loadModel(const String &model_path);

            void setSize(Vector2 size);
            void addStrokePoint(int stroke, Vector2 point);
            void clear();

            Array matchStrokes(int max_amount);
            static Array orderIndependentStrokeMatch(const String &model_path, int max_amount_per_match, Vector2 size, const Array strokes);
    };
}

#endif
