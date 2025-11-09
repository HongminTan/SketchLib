// 最简用法示例：UnivMon（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "FlowKey.h"
#include "UnivMon.h"

int main() {
    {
        UnivMon<OneTuple> um(3, 4096);
        OneTuple a(0x01020304);
        um.update(a, 100);
        std::cout << "query=" << um.query(a)
                  << " layer_count=" << um.get_layer_count()
                  << " memory_budget=" << um.get_memory_budget()
                  << " backend=" << (int)um.get_backend() << std::endl;
    }
    {
        UnivMon<TwoTuple> um(3, 4096);
        TwoTuple f(0x0a000001, 0x0a000002);
        um.update(f, 100);
        std::cout << "query=" << um.query(f)
                  << " layer_count=" << um.get_layer_count()
                  << " memory_budget=" << um.get_memory_budget()
                  << " backend=" << (int)um.get_backend() << std::endl;
    }
    {
        UnivMon<FiveTuple> um(3, 8192);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        um.update(t, 100);
        std::cout << "query=" << um.query(t)
                  << " layer_count=" << um.get_layer_count()
                  << " memory_budget=" << um.get_memory_budget()
                  << " backend=" << (int)um.get_backend() << std::endl;
    }
    return 0;
}
