// 最简用法示例：FlowRadar（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "FlowKey.h"
#include "FlowRadar.h"

int main() {
    {
        FlowRadar<OneTuple> fr(8192, 0.3, 3, 3);
        OneTuple a(0x01020304);
        fr.update(a, 10);
        std::cout << "query=" << fr.query(a)
                  << " bf_hashes=" << fr.get_bf_num_hashes()
                  << " ct_hashes=" << fr.get_ct_num_hashes()
                  << " table_size=" << fr.get_table_size() << std::endl;
    }
    {
        FlowRadar<TwoTuple> fr(8192, 0.3, 3, 3);
        TwoTuple f(0x0a000001, 0x0a000002);
        fr.update(f, 5);
        std::cout << "query=" << fr.query(f)
                  << " bf_hashes=" << fr.get_bf_num_hashes()
                  << " ct_hashes=" << fr.get_ct_num_hashes()
                  << " table_size=" << fr.get_table_size() << std::endl;
    }
    {
        FlowRadar<FiveTuple> fr(16384, 0.3, 4, 3);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        fr.update(t, 7);
        std::cout << "query=" << fr.query(t)
                  << " bf_hashes=" << fr.get_bf_num_hashes()
                  << " ct_hashes=" << fr.get_ct_num_hashes()
                  << " table_size=" << fr.get_table_size() << std::endl;
    }
    return 0;
}
