#pragma once

namespace benchmark {
    class IVarInt {
    public:
        using int_type_t = long long;

        virtual ~IVarInt() {}

        virtual bool done() = 0;
        virtual int_type_t getNext() = 0;
    };

    class VarIntLinear : public IVarInt {
        int_type_t _value;
        int_type_t _to;
        int_type_t _step;

    public:
        VarIntLinear(int_type_t from, int_type_t to, int_type_t step) :
                _value(from),
                _to(to),
                _step(step) {
        }

        bool done() override {
            return _value > _to;
        }

        int_type_t getNext() override {
            int_type_t ret = _value;
            _value += _step;
            return ret;
        }
    };

    class VarIntLog2 : public IVarInt {
        using int_type_t = long long;
        int_type_t _value;
        int_type_t _to;

    public:
        VarIntLog2(int_type_t from, int_type_t to) :
                _value(from),
                _to(to) {
        }

        bool done() override {
            return _value > _to;
        }

        int_type_t getNext() override {
            int_type_t ret = _value;
            _value *= 2;
            return ret;
        }
    };

    class VarIntLog10 : public IVarInt {
        using int_type_t = long long;
        int_type_t _value;
        int_type_t _to;

    public:
        VarIntLog10(int_type_t from, int_type_t to) :
                _value(from),
                _to(to) {
        }

        bool done() override {
            return _value > _to;
        }

        int_type_t getNext() override {
            int_type_t ret = _value;
            _value *= 10;
            return ret;
        }
    };
}