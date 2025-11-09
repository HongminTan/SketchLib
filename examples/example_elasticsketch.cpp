// 最简用法示例：ElasticSketch（OneTuple/TwoTuple/FiveTuple）
#include <iostream>
#include "ElasticSketch.h"
#include "FlowKey.h"

int main() {
    {
        ElasticSketch<OneTuple> es(1000, 2, 4096, 8);
        OneTuple a(0x01020304);
        es.update(a, 100);
        auto ls = es.get_light_size();
        std::cout << "query=" << es.query(a)
                  << " heavy_buckets=" << es.get_heavy_bucket_count()
                  << " light_rows=" << ls.first << " light_cols=" << ls.second
                  << " lambda=" << es.get_lambda() << std::endl;
    }
    {
        ElasticSketch<TwoTuple> es(1000, 2, 4096, 8);
        TwoTuple f(0x0a000001, 0x0a000002);
        es.update(f, 100);
        auto ls = es.get_light_size();
        std::cout << "query=" << es.query(f)
                  << " heavy_buckets=" << es.get_heavy_bucket_count()
                  << " light_rows=" << ls.first << " light_cols=" << ls.second
                  << " lambda=" << es.get_lambda() << std::endl;
    }
    {
        ElasticSketch<FiveTuple> es(2000, 2, 8192, 8);
        FiveTuple t(0x0a000001, 0x08080808, 1234, 80, 6);
        es.update(t, 100);
        auto ls = es.get_light_size();
        std::cout << "query=" << es.query(t)
                  << " heavy_buckets=" << es.get_heavy_bucket_count()
                  << " light_rows=" << ls.first << " light_cols=" << ls.second
                  << " lambda=" << es.get_lambda() << std::endl;
    }
    return 0;
}
