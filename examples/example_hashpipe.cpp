// 最简用法示例：创建、更新、查询（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "FlowKey.h"
#include "HashPipe.h"

int main() {
    {
        HashPipe<OneTuple> hp(4096, 4);
        OneTuple a(0x01020304);
        hp.update(a, 25);
        std::cout << "query=" << hp.query(a)
                  << " stages=" << hp.get_num_stages()
                  << " buckets_per_stage=" << hp.get_buckets_per_stage()
                  << std::endl;
    }
    {
        HashPipe<TwoTuple> hp(4096, 4);
        TwoTuple f(0x0a000001, 0x0a000002);
        hp.update(f, 35);
        std::cout << "query=" << hp.query(f)
                  << " stages=" << hp.get_num_stages()
                  << " buckets_per_stage=" << hp.get_buckets_per_stage()
                  << std::endl;
    }
    {
        HashPipe<FiveTuple> hp(8192, 4);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        hp.update(t, 10);
        std::cout << "query=" << hp.query(t)
                  << " stages=" << hp.get_num_stages()
                  << " buckets_per_stage=" << hp.get_buckets_per_stage()
                  << std::endl;
    }
    return 0;
}
