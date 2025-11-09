// 最简用法示例：创建、更新、查询（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "CountMin.h"
#include "FlowKey.h"

int main() {
    {
        CountMin<OneTuple> cm(4, 1024);
        OneTuple a(0x01020304);
        cm.update(a, 10);
        std::cout << "query=" << cm.query(a) << " rows=" << cm.get_rows()
                  << " cols=" << cm.get_cols() << std::endl;
    }
    {
        CountMin<TwoTuple> cm(4, 1024);
        TwoTuple f(0x0a000001, 0x0a000002);
        cm.update(f, 5);
        std::cout << "query=" << cm.query(f) << " rows=" << cm.get_rows()
                  << " cols=" << cm.get_cols() << std::endl;
    }
    {
        CountMin<FiveTuple> cm(4, 2048);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        cm.update(t, 7);
        std::cout << "query=" << cm.query(t) << " rows=" << cm.get_rows()
                  << " cols=" << cm.get_cols() << std::endl;
    }
    return 0;
}
