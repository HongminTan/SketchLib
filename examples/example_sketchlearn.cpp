// 最简用法示例：SketchLearn（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "FlowKey.h"
#include "SketchLearn.h"

int main() {
    {
        SketchLearn<OneTuple> sl(4096, 2, 0.5);
        OneTuple a(0x01020304);
        sl.update(a, 10);
        std::cout << "query=" << sl.query(a) << " rows=" << sl.get_num_rows()
                  << " cols=" << sl.get_num_cols()
                  << " theta=" << sl.get_theta() << std::endl;
    }
    {
        SketchLearn<TwoTuple> sl(4096, 2, 0.5);
        TwoTuple f(0x0a000001, 0x0a000002);
        sl.update(f, 5);
        std::cout << "query=" << sl.query(f) << " rows=" << sl.get_num_rows()
                  << " cols=" << sl.get_num_cols() << std::endl;
    }
    {
        SketchLearn<FiveTuple> sl(8192, 2, 0.5);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        sl.update(t, 7);
        std::cout << "query=" << sl.query(t) << std::endl;
    }
    return 0;
}
