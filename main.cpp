#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <random> 

#define FOR(i,N) for(long i=0;i<N;i++)
//#define NO_EXCEPTION
using namespace std;
decltype(std::chrono::high_resolution_clock::now()) start_time;
typedef decltype(std::chrono::high_resolution_clock::now()) time_type;
const auto& now = std::chrono::high_resolution_clock::now;
std::default_random_engine random_generator(123457);

void tic() {
    start_time = std::chrono::high_resolution_clock::now();
}
void toc(string event) {
    auto t = chrono::high_resolution_clock::now();
    auto dt = chrono::duration_cast<std::chrono::nanoseconds>(t - start_time).count();
    cout << event << ": " << dt << " nanosec" << endl;
    start_time = t;
}
long timedif(time_type t1, time_type t2) {
    return chrono::duration_cast<std::chrono::microseconds>(t1 - t2).count();
}



long num_video, num_user, num_req, num_cache, size_cache, total_req_count = 0;
vector<int> size_videos;
typedef long Delay;
typedef long VideoID;
typedef long CacheID;
typedef long UserID;
vector<map<CacheID, Delay>> user_cache;
struct Request {
    VideoID video;
    UserID user;
    long number;
};
vector<Request> reqs;
void read_file(string file);
inline Delay get_cache_user_delay(CacheID c, UserID u) {
    return user_cache[u][c];
}
template <typename T>
std::vector<size_t> sort_index_large_to_small(std::vector<T> const& values) {
    std::vector<size_t> indices(values.size());
    std::iota(begin(indices), end(indices), 0);
    std::sort(begin(indices), end(indices),
        [&](size_t a, size_t b) { return values[a] > values[b]; }
    );
    return indices;
}

class ConnectionStatus {
public:
    vector<set<UserID>> cache_to_user;
    vector<map<VideoID, int>> user_reqs;
    vector<vector<VideoID>> cache_may_have_video;
    vector<vector<set<UserID>>> cache_video_to_possible_user;

    vector<map<VideoID, CacheID>> user_video_from_where;
    vector<set<VideoID>> cache_load_set;
    vector<vector<set<UserID>>> cache_video_to_where;
    vector<set<CacheID>> video_exist_in_cache;
    vector<int> cache_current_volume;
    double score;
    ConnectionStatus() {
        cache_to_user.resize(num_cache);
        FOR(user_id, num_user)
            for (auto& it : user_cache[user_id]) {
                CacheID c = it.first; Delay time = it.second;
                if (c != -1) cache_to_user[c].insert(user_id);
            }

        user_reqs.resize(num_user);
        for (auto& r : reqs)
            user_reqs[r.user][r.video] += r.number;

        cache_may_have_video.resize(num_cache);
        vector<set<VideoID>> cache_may_have_video_set_version;
        cache_may_have_video_set_version.resize(num_cache);
        FOR(cache_id, num_cache)
            for (UserID user_id : cache_to_user[cache_id])
                for (auto key_value : user_reqs[user_id]) {
                    VideoID video_id = key_value.first;
                    cache_may_have_video_set_version[cache_id].insert(video_id);
                }
        FOR(cache_id, num_cache) {
            cache_may_have_video[cache_id].assign(cache_may_have_video_set_version[cache_id].begin(), cache_may_have_video_set_version[cache_id].end());
        }
        vector<set<UserID>> temp(num_video);
        cache_video_to_possible_user.resize(num_cache, temp);
        //for (auto& it : cache_video_to_possible_user)
        //    it.resize(num_video);
        FOR(cache_id, num_cache)
            for (UserID user_id : cache_to_user[cache_id])
                for (auto& video_delay : user_reqs[user_id])
                    cache_video_to_possible_user[cache_id][video_delay.first].insert(user_id);

        user_video_from_where.resize(num_user);
        for (auto& r : reqs)
            user_video_from_where[r.user][r.video] = -1; //from server

        cache_load_set.resize(num_cache);

        cache_video_to_where.resize(num_cache);
        for (auto& it : cache_video_to_where)
            it.resize(num_video);

        video_exist_in_cache.resize(num_video);

        cache_current_volume.resize(num_cache);
        score = 0.0;


    }

    double score_changes_if_add_video_to_cache(VideoID video_id, CacheID cache_id) {
        //time_type t0, t1, t2, t3, t4;
        double score_change = 0.0;
        for (UserID user_id : cache_video_to_possible_user[cache_id][video_id]) {
            //t0 = now();
            auto it = user_video_from_where[user_id].find(video_id);
            //t1 = now();
            if (it != user_video_from_where[user_id].end()) {
                CacheID old_cache_id = it->second;
                Delay delta = get_cache_user_delay(old_cache_id, user_id) - get_cache_user_delay(cache_id, user_id);
                if (delta >= 0) {//added video is closer to user
                    score_change += user_reqs[user_id][video_id] * delta;
                }
            }
            //t2 = now();
            //cout << "t012=" << timedif(t1, t0) << " " << timedif(t2, t1) << endl;
        }
        return score_change * 1000.0 / total_req_count;
    }

    void add_video_to_cache(VideoID video_id, CacheID cache_id) {
        auto& cache_set = cache_load_set[cache_id];
        auto& video_existance = video_exist_in_cache[video_id];
        if (!cache_set.insert(video_id).second)
#ifdef NO_EXCEPTION
            ;
#else
            throw "video already exist";
#endif
        if (!video_existance.insert(cache_id).second)
#ifdef NO_EXCEPTION
            ;
#else
            throw "video already exist";
#endif
        if ((cache_current_volume[cache_id] += size_videos[video_id]) > size_cache)
#ifdef NO_EXCEPTION
            ;
#else

            throw "cache is full";
#endif


        double score_change = 0.0;
        for (UserID user_id : cache_video_to_possible_user[cache_id][video_id]) {
            auto it = user_video_from_where[user_id].find(video_id);
            if (it != user_video_from_where[user_id].end()) {
                CacheID old_cache_id = it->second;
                Delay delta = get_cache_user_delay(old_cache_id, user_id) - get_cache_user_delay(cache_id, user_id);
                if (delta >= 0) {//added video is closer to user
                    //add new connection
                    if (!cache_video_to_where[cache_id][video_id].insert(user_id).second)
#ifdef NO_EXCEPTION
                        ;
#else
                        throw "error";
#endif
                    //break old connection
                    if (old_cache_id != -1)
                        cache_video_to_where[old_cache_id][video_id].erase(user_id);
                    score_change += user_reqs[user_id][video_id] * delta;
                    user_video_from_where[user_id][video_id] = cache_id;
                }
            }
        }
        score += score_change * 1000.0 / total_req_count;
    }

    void del_video_from_cache(VideoID video_id, CacheID cache_id) {
        auto& cache_set = cache_load_set[cache_id];
        auto& video_existance = video_exist_in_cache[video_id];

        if (cache_set.erase(video_id) == 0)
#ifdef NO_EXCEPTION
            cout << "error" << endl;
#else
            throw "error";
#endif
        if (video_existance.erase(cache_id) == 0)
#ifdef NO_EXCEPTION
            ;
#else
            throw "error";
#endif
        if ((cache_current_volume[cache_id] -= size_videos[video_id]) < 0)
#ifdef NO_EXCEPTION
            ;
#else
            throw "cache volume negative!";
#endif


        double score_change = 0.0;
        auto& user_set = cache_video_to_where[cache_id][video_id];
        for (UserID user_id : user_set) {
            auto& user_caches_info = user_cache[user_id];
            set<CacheID> all_cache_connected_to_user;
            for (auto& it : user_caches_info) all_cache_connected_to_user.insert(it.first);
            set<CacheID> possible_cache;
            set_intersection(all_cache_connected_to_user.begin(), all_cache_connected_to_user.end(),
                video_existance.begin(), video_existance.end(), inserter(possible_cache, possible_cache.begin()));
            CacheID dest_cache = -1;
            Delay min_time = user_cache[user_id][-1];
            for (CacheID c : possible_cache) {
                Delay time = get_cache_user_delay(c, user_id);
                if (time < min_time) {
                    dest_cache = c;
                    min_time = time;
                }
            }
            if (dest_cache != -1)
                if (!cache_video_to_where[dest_cache][video_id].insert(user_id).second)
#ifdef NO_EXCEPTION
                    ;
#else
                    throw "error";
#endif
            score_change += user_reqs[user_id][video_id] * (
                get_cache_user_delay(cache_id, user_id) - min_time
                );
            user_video_from_where[user_id][video_id] = dest_cache;
        }
        cache_video_to_where[cache_id][video_id].clear();
        score += score_change * 1000.0 / total_req_count;
    }

    void fill_cache_by_best_videos(CacheID cache_id) {
        time_type t0, t1, t2, t3;
        vector<VideoID>& video_list = cache_may_have_video[cache_id];
        vector<double> video_score(video_list.size(), 0.0);
        cout << "c=" << cache_id << " may have video = " << video_list.size() << endl;
        t0 = now();
        FOR(i, video_list.size()) {
            VideoID v = video_list[i];
            double score_change = score_changes_if_add_video_to_cache(v, cache_id);
            video_score[i] = score_change / size_videos[v];
        }
        std::vector<size_t> indices = sort_index_large_to_small(video_score);
        long insert_count = 0;
        for (auto index : indices) {
            VideoID v_id = video_list[index];
            if (video_score[index] == 0) break;
            long rest_volume = size_cache - cache_current_volume[cache_id];
            if (size_videos[v_id] <= rest_volume) {
                add_video_to_cache(v_id, cache_id);
                insert_count += 1;
            }
        }
        cout << "inserted video:" << insert_count << endl;
        cout << "rest_volume: " << size_cache - cache_current_volume[cache_id] << endl;
    }

    double calculate_score() {
        long long save_time = 0;
        long total_req = 0;
        for (auto r : reqs) {
            CacheID c = user_video_from_where[r.user][r.video];
            save_time += r.number * (user_cache[r.user][-1] - user_cache[r.user][c]);
            total_req += r.number;
        }
        return 1000.0 * save_time / total_req;
    }

    void empty_cache(CacheID cache_id) {
        while (!cache_load_set[cache_id].empty()) {
            VideoID v = *(cache_load_set[cache_id].begin());
            del_video_from_cache(v, cache_id);
        }
    }

    void stupid_method() {
        time_type t0, t1, t2;
        FOR(c, num_cache) {
            t0 = now();
            fill_cache_by_best_videos(c);
            t1 = now();
            //cout << "c=" << c << " t=" << timedif(t1, t0) / 1000 << "microsecs" << endl;
            cout << "score = " << score << endl;
        }
        submission("out.out");
        FOR(i, 50) {
            std::vector<size_t> indices(num_cache);
            std::iota(begin(indices), end(indices), 0);
            //shuffle(indices.begin(), indices.end(), random_generator);
            for (auto c : indices) {
                empty_cache(c);
                fill_cache_by_best_videos(c);
                cout << "score = " << score << endl;
            }
            submission("out.out");
        }
    }

    void update_best_cache(
        vector<vector<bool>>& video_inserted_to_cache,
        vector<CacheID>& best_cache_to_insert_for_video,
        vector<double>& score_for_best_cache_to_insert_for_video,
        vector<set<VideoID>>& videos_choose_cache,
        VideoID v) {
        CacheID best_cache_id = -1;
        double best_score = -1.0;
        if (best_cache_to_insert_for_video[v] >= 0) {
            if (videos_choose_cache[best_cache_to_insert_for_video[v]].erase(v) == 0) throw "not there";
        }
        FOR(c, num_cache) {
            if (!video_inserted_to_cache[v][c] && cache_current_volume[c] + size_videos[v] <= size_cache) {
                double move_score = score_changes_if_add_video_to_cache(v, c);
                if (move_score > 0 && move_score > best_score) {
                    best_score = move_score; best_cache_id = c;
                }
            }
        }
        best_cache_to_insert_for_video[v] = best_cache_id;
        score_for_best_cache_to_insert_for_video[v] = best_score;
        if (best_cache_id != -1)
            if (!videos_choose_cache[best_cache_id].insert(v).second) throw "impossible";
    }
    void stupid_method2() {
        CacheID best_cache_id;
        VideoID best_video_id;
        double best_score;
        long long counter = 0;
        vector<vector<bool>> video_inserted_to_cache(num_video, vector<bool>(num_cache, false));
        vector<CacheID> best_cache_to_insert_for_video(num_video, -1);
        vector<double> score_for_best_cache_to_insert_for_video(num_video, -1.0);
        vector<set<VideoID>> videos_choose_cache(num_cache);

        FOR(v, num_video) {
            update_best_cache(video_inserted_to_cache, best_cache_to_insert_for_video, score_for_best_cache_to_insert_for_video, videos_choose_cache, v);
        }

        auto sort_index_fun = [&](VideoID v1, VideoID v2)->bool {
            return score_for_best_cache_to_insert_for_video[v1] > score_for_best_cache_to_insert_for_video[v2];
        };
        vector<size_t> video_index(num_video);
        iota(begin(video_index), end(video_index), 0);


        while (1) {
            time_type t0, t1, t2, t3;
            t0 = now();
            best_video_id = -1;
            best_cache_id = -1;

            double best_score = 0.0;
            sort(begin(video_index), end(video_index), sort_index_fun);
            t1 = now();
            best_video_id = video_index[0];
            best_cache_id = best_cache_to_insert_for_video[best_video_id];
            best_score = score_for_best_cache_to_insert_for_video[best_video_id];
            if (best_cache_id == -1) break;

            add_video_to_cache(best_video_id, best_cache_id);
            video_inserted_to_cache[best_video_id][best_cache_id] = true;
            t2 = now();
            update_best_cache(video_inserted_to_cache, best_cache_to_insert_for_video, score_for_best_cache_to_insert_for_video, videos_choose_cache, best_video_id);
            vector<VideoID> vec_videos_choose_cache(videos_choose_cache[best_cache_id].begin(), videos_choose_cache[best_cache_id].end());
            cout << vec_videos_choose_cache.size() << " videos to update" << endl;
            for (VideoID video_id : vec_videos_choose_cache) {
                update_best_cache(video_inserted_to_cache, best_cache_to_insert_for_video, score_for_best_cache_to_insert_for_video, videos_choose_cache, video_id);
            }
            t3 = now();
            cout << counter << ": video " << best_video_id << " into cache " << best_cache_id << " score " << best_score << endl;
            cout << "score " << score << endl;
            cout << "time: " << timedif(t1, t0) << " " << timedif(t2, t1) << " " << timedif(t3, t2) << endl;
            counter++;
        }
    }
    void stupid_method3() {
        CacheID best_cache_id;
        VideoID best_video_id;
        double best_score;
        long long counter = 0;
        vector<vector<bool>> video_inserted_to_cache(num_video, vector<bool>(num_cache, false));

        while (1) {
            best_video_id = -1;
            best_cache_id = -1;
            double best_score = 0.0;
            FOR(v, num_video) {
                FOR(c, num_cache) {
                    if (!video_inserted_to_cache[v][c] && cache_current_volume[c] + size_videos[v] <= size_cache) {
                        double move_score = score_changes_if_add_video_to_cache(v, c);
                        if (move_score > best_score) {
                            best_score = move_score;
                            best_cache_id = c;
                            best_video_id = v;
                        }
                    }
                }
            }
            if (best_cache_id != -1) {
                add_video_to_cache(best_video_id, best_cache_id);
                video_inserted_to_cache[best_video_id][best_cache_id] = true;
                cout << counter << ": video " << best_video_id << " into cache " << best_cache_id << " score " << best_score << endl;
                cout << "score " << score << endl;
                counter++;
            }
            else {
                break;
            }
        }
    }
    void submission(string outfile) {
        ofstream f(outfile);
        f << num_cache << endl;
        FOR(i, num_cache) {
            f << i << " ";
            for (VideoID v : cache_load_set[i]) f << v << " ";
            f << endl;
        }
        f.close();
    }
    void read_submission(string file) {
        ifstream f(file);
        int num_cache_from_file;
        f >> num_cache_from_file;
        if (num_cache_from_file != num_cache) throw "error";
        FOR(i, num_cache) {
            int video;
            f >> video;
            add_video_to_cache(video, i);
        }
        f.close();
    }
};


long main() {
    string file = "trending_today.in";
    //string file = "me_at_the_zoo.in";
    //string file = "videos_worth_spreading.in";
    //string file = "kittens.in";
    read_file(file);
    ConnectionStatus cs = ConnectionStatus();
    time_type t0, t1;
    t0 = now();
    cs.stupid_method();
    t1 = now();
    cout << "time lapse = " << timedif(t1, t0) / 1000000.0 << "secs" << endl;
    cout << "calculated score=" << cs.calculate_score() << endl;
    cout << "score = " << cs.score << endl;
    cs.submission(file + "out");
    return 0;
}

void read_file(string file) {
    ifstream f(file);
    f >> num_video >> num_user >> num_req >> num_cache >> size_cache;
    size_videos.resize(num_video);
    FOR(i, num_video)
        f >> size_videos[i];
    user_cache.resize(num_user);
    FOR(i, num_user) {
        Delay server_delay;        f >> server_delay;
        long cache_count;           f >> cache_count;
        auto& dict = user_cache[i];
        dict[-1] = server_delay;
        for (long c = 0; c < cache_count; c++) {
            CacheID cache_id;
            f >> cache_id;
            Delay delay;
            f >> delay;
            dict[cache_id] = delay;
        }
    }
    reqs.resize(num_req);
    FOR(i, num_req) {
        f >> reqs[i].video >> reqs[i].user >> reqs[i].number;
        total_req_count += reqs[i].number;
    }
    f.close();
}