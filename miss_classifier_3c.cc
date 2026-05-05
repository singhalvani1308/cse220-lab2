
#include "miss_classifier_3c.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <vector>



class LRUCache {
public:
    LRUCache() : num_sets_(0), assoc_(0) {}

    void init(uint64_t num_sets, uint32_t assoc) {
        num_sets_ = num_sets;
        assoc_    = assoc;
        sets_.resize(num_sets_);
        maps_.resize(num_sets_);
    }

    bool access(uint64_t line_addr) {
        uint64_t set_idx = line_addr % num_sets_;
        auto& lru_list = sets_[set_idx];
        auto& lru_map  = maps_[set_idx];

        auto it = lru_map.find(line_addr);
        if (it != lru_map.end()) {
            
            lru_list.erase(it->second);
            lru_list.push_front(line_addr);
            it->second = lru_list.begin();
            return true;
        }

        
        if ((uint32_t)lru_list.size() >= assoc_) {
            
            uint64_t evict = lru_list.back();
            lru_list.pop_back();
            lru_map.erase(evict);
        }
        lru_list.push_front(line_addr);
        lru_map[line_addr] = lru_list.begin();
        return false;
    }

    bool contains(uint64_t line_addr) const {
        uint64_t set_idx = line_addr % num_sets_;
        return maps_[set_idx].count(line_addr) > 0;
    }

private:
    uint64_t num_sets_;
    uint32_t assoc_;
    
    std::vector<std::list<uint64_t>>                                  sets_;
    std::vector<std::unordered_map<uint64_t,
                    std::list<uint64_t>::iterator>>                   maps_;
};



static uint64_t g_cache_size_bytes  = 0;
static uint32_t g_assoc             = 0;
static uint32_t g_line_size_bytes   = 64;
static uint64_t g_num_lines         = 0;
static uint64_t g_num_sets          = 0;


static std::unordered_set<uint64_t> g_seen_lines;     
static LRUCache                     g_fa_cache;        
static LRUCache                     g_sa_cache;        


static uint64_t g_cnt_compulsory = 0;
static uint64_t g_cnt_capacity   = 0;
static uint64_t g_cnt_conflict   = 0;
static uint64_t g_cnt_total      = 0;



extern "C" void miss_classifier_3c_init(uint64_t cache_size_bytes,
                                         uint32_t assoc,
                                         uint32_t line_size_bytes)
{
    g_cache_size_bytes = cache_size_bytes;
    g_assoc            = assoc;
    g_line_size_bytes  = line_size_bytes;
    g_num_lines        = cache_size_bytes / line_size_bytes;
    g_num_sets         = g_num_lines / assoc;

    
    g_fa_cache.init(1, (uint32_t)g_num_lines);

    
    g_sa_cache.init(g_num_sets, assoc);

    g_seen_lines.clear();
    g_cnt_compulsory = g_cnt_capacity = g_cnt_conflict = g_cnt_total = 0;

    fprintf(stderr,
        "[3C] init: size=%lu bytes, assoc=%u, line=%u B -> %lu sets, %lu lines\n",
        (unsigned long)cache_size_bytes, assoc, line_size_bytes,
        (unsigned long)g_num_sets, (unsigned long)g_num_lines);
}

extern "C" void classify_dcache_miss(uint64_t addr)
{
    
    uint64_t line_addr = addr / g_line_size_bytes;

    g_cnt_total++;

    
    if (g_seen_lines.find(line_addr) == g_seen_lines.end()) {
        g_seen_lines.insert(line_addr);
        g_cnt_compulsory++;

        
        g_fa_cache.access(line_addr);
        g_sa_cache.access(line_addr);
        return;
    }

    
    
    
    bool fa_hit = g_fa_cache.access(line_addr);
    if (!fa_hit) {
        g_cnt_capacity++;
        
        g_sa_cache.access(line_addr);
        return;
    }

    
    
    bool sa_hit = g_sa_cache.access(line_addr);
    if (!sa_hit) {
        g_cnt_conflict++;
        return;
    }

    
    
    
    g_cnt_conflict++;
}

extern "C" void miss_classifier_3c_dump_stats(void)
{
    
    
    
    printf("dcache_3c_total\t\t%lu\n",      (unsigned long)g_cnt_total);
    printf("dcache_3c_compulsory\t%lu\n",   (unsigned long)g_cnt_compulsory);
    printf("dcache_3c_capacity\t%lu\n",     (unsigned long)g_cnt_capacity);
    printf("dcache_3c_conflict\t%lu\n",     (unsigned long)g_cnt_conflict);

    if (g_cnt_total > 0) {
        fprintf(stderr, "[3C] total=%lu  compulsory=%.1f%%  capacity=%.1f%%  conflict=%.1f%%\n",
            (unsigned long)g_cnt_total,
            100.0 * g_cnt_compulsory / g_cnt_total,
            100.0 * g_cnt_capacity   / g_cnt_total,
            100.0 * g_cnt_conflict   / g_cnt_total);
    }
}

extern "C" uint64_t miss_classifier_get_compulsory(void) { return g_cnt_compulsory; }
extern "C" uint64_t miss_classifier_get_capacity(void)   { return g_cnt_capacity;   }
extern "C" uint64_t miss_classifier_get_conflict(void)   { return g_cnt_conflict;   }
extern "C" uint64_t miss_classifier_get_total(void)      { return g_cnt_total;       }
