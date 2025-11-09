// 最简用法示例：创建、更新、查询（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "CountSketch.h"
#include "FlowKey.h"

int main() {
    {
        CountSketch<OneTuple> cs(5, 2048);
        OneTuple a(0x01020304);
        cs.update(a, 10);
        std::cout << "query=" << cs.query(a) << " rows=" << cs.get_rows()
                  << " cols=" << cs.get_cols() << std::endl;
    }
    {
        CountSketch<TwoTuple> cs(5, 2048);
        TwoTuple f(0x0a000001, 0x0a000002);
        cs.update(f, -3);
        cs.update(f, 5);
        std::cout << "query=" << cs.query(f) << " rows=" << cs.get_rows()
                  << " cols=" << cs.get_cols() << std::endl;
    }
    {
        CountSketch<FiveTuple> cs(5, 4096);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        cs.update(t, 7);
        std::cout << "query=" << cs.query(t) << " rows=" << cs.get_rows()
                  << " cols=" << cs.get_cols() << std::endl;
    }
    return 0;
}
