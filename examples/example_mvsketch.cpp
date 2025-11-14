// 最简用法示例：创建、更新、查询（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "FlowKey.h"
#include "MVSketch.h"

int main() {
    {
        MVSketch<OneTuple> mv(4, 1024);
        OneTuple a(0x01020304);
        mv.update(a, 10);
        std::cout << "query=" << mv.query(a) << " rows=" << mv.get_rows()
                  << " cols=" << mv.get_cols() << std::endl;
    }
    {
        MVSketch<TwoTuple> mv(4, 2048);
        TwoTuple f(0x0a000001, 0x0a000002);
        mv.update(f, 5);
        std::cout << "query=" << mv.query(f) << " rows=" << mv.get_rows()
                  << " cols=" << mv.get_cols() << std::endl;
    }
    {
        MVSketch<FiveTuple> mv(5, 4096);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        mv.update(t, 7);
        std::cout << "query=" << mv.query(t) << " rows=" << mv.get_rows()
                  << " cols=" << mv.get_cols() << std::endl;
    }
    return 0;
}
