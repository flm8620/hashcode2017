import numpy as np
import pickle
import os
import sys
import time


### Rq : 1. use space not TAB
###      2. python2.7

###-- 1. File Read ###
# input  : file_path = '.'      file_name = 'filename.txt'
# outpout : num_video - int
#               num_user -int
#               num_req   -int
#               num_cache -int
#               size_cache - int
#               size_videos - np.array - contains the size of each videos
#               user_cache - list of dicts - contains each user's info (dict: key - cacheID  value - cacheDelay)
#               reqs - list of lists - contains each request's info


def Read_file(file_path, file_name):
  fp = open(os.path.join(file_path, file_name), 'r')
  fp_line = fp.read().split('\n')

  num_video, num_user, num_req, num_cache, size_cache = [int(x) for x in fp_line[0].split(' ')]

  ## read video size
  size_videos = np.array([int(x) for x in fp_line[1].split(' ')])

  user_cache = []
  index = 2

  ## read endpoint information
  for i in range(num_user):
    item = fp_line[index].split(' ')
    index += 1

    dict_cache = {}
    dict_cache[-1] = int(item[0])

    for j in range(int(item[1])):
      dict_cache[int(fp_line[index].split(' ')[0])] = int(fp_line[index].split(' ')[1])
      index += 1

    user_cache.append(dict_cache)
  total_req_count = 0
  ## read request information
  reqs = []
  for i in range(num_req):
    reqs.append(tuple([int(x) for x in fp_line[index].split(' ')]))
    index += 1
    total_req_count += reqs[i][2]

  return num_video, num_user, num_req, num_cache, size_cache, size_videos, user_cache, reqs, total_req_count


def cache_have_which_user():
  cache_user = []
  for i in range(num_cache):
    cache_user.append(set())
  for u_id in range(num_user):
    dict_cache = user_cache[u_id]
    for cache_id, time in dict_cache.iteritems():
      if cache_id == -1:
        continue
      cache_user[cache_id].add(u_id)
  return cache_user


def init_user_video_from_where():
  user_video_from_where = []
  for u_id in range(num_user):
    user_video_from_where.append({})
  for (video, endpoint, nb_request) in reqs:
    user_video_from_where[endpoint][video] = -1  # by default from main server
  return user_video_from_where


def calculate_user_reqs():
  user_reqs = []
  for u_id in range(num_user):
    user_reqs.append({})
  for (video, endpoint, nb_request) in reqs:
    if video in user_reqs[endpoint]:
      user_reqs[endpoint][video] += nb_request
    else:
      user_reqs[endpoint][video] = nb_request
  return user_reqs


def calculate_distance_cache_user():
  distance_cache_user = {}
  for u_id in range(num_user):
    dict_cache = user_cache[u_id]
    for cache_id, time in dict_cache.iteritems():
      distance_cache_user[(cache_id, u_id)] = time
  return distance_cache_user


class ConnectionStatus:
  def __init__(self):
    # cache_to_user[cache_id] = set(user1, user2, ...)
    # const
    self.cache_to_user = cache_have_which_user()

    # distance_cache_user = dict{ (cache_i1, user_j1) : time1, (cache_i2, user_j2) : time2  }
    # cache_i = -1 means main server
    # const
    self.distance_cache_user = calculate_distance_cache_user()

    # user_reqs[user_id] = dict{video1 : req_num1, video2 : req_num2, ...}
    # const
    self.user_reqs = calculate_user_reqs()

    # cache_may_have_video[cache_id] = [v1,v3,v6, ...]
    # const
    self.cache_may_have_video = [[] for c in range(num_cache)]
    self.init_cache_may_have_video()

    # cache_video_to_possible_user[cache][video] = set(users)
    # const
    self.cache_video_to_possible_user = []
    for i in range(num_cache):
      self.cache_video_to_possible_user.append([set() for j in range(num_video)])
    self.init_cache_video_to_possible_user()

    ############ mutable vars #############

    # user_video_from_where[user_id] = dict{video1: cache1, video2: cache2, ... }
    # cache_id == -1 means main server
    # mutable
    self.user_video_from_where = init_user_video_from_where()

    # cache_load_set[cache_id] = set(video1, video2, ...)
    # mutable
    self.cache_load_set = [set() for i in range(num_cache)]

    # cache_video_to_where[cache_id][video_id] = set(user1, user2, ...)
    # mutable
    self.cache_video_to_where = []
    for i in range(num_cache):
      self.cache_video_to_where.append([set() for j in range(num_video)])

    # video_exist_in_cache[video_id] = set(cache1, cache2, ...) # empty means only in main server
    # mutable
    self.video_exist_in_cache = [set() for i in range(num_video)]

    # cache_current_volume[cache_id] = current volume
    # mutable
    self.cache_current_volume = [0 for i in range(num_cache)]

    # mutable
    self.score = 0.0

  def init_cache_may_have_video(self):
    for cache, user_set in enumerate(self.cache_to_user):
      for user in user_set:
        for video in self.user_reqs[user].keys():
          self.cache_may_have_video[cache].append(video)
    for c in range(num_cache):
      self.cache_may_have_video[c] = list(set(self.cache_may_have_video[c]))

  def init_cache_video_to_possible_user(self):
    for cache_id in range(num_cache):
      for user_id in self.cache_to_user[cache_id]:
        user_need_videos = self.user_reqs[user_id]
        for video_id in user_need_videos.keys():
          self.cache_video_to_possible_user[cache_id][video_id].add(user_id)

  def score_changes_if_add_video_to_cache(self, video_id, cache_id):
    score_change = 0.0

    for user_id in self.cache_video_to_possible_user[cache_id][video_id]:
      user_need_videos = self.user_video_from_where[user_id]
      if video_id in user_need_videos:
        # if this user want this video
        old_cache_id = user_need_videos[video_id]
        if user_cache[user_id][cache_id] < user_cache[user_id][old_cache_id]:
          # if added video is closer to user

          # added video will get this new user

          # add new connection
          # if user_id in self.cache_video_to_where[cache_id][video_id]:
          #   raise "error"

          score_change += self.user_reqs[user_id][video_id] * (
            user_cache[user_id][old_cache_id] - user_cache[user_id][cache_id]
          )

    return score_change * 1000.0 / total_req_count

  def add_video_to_cache(self, video_id, cache_id):
    if video_id in self.cache_load_set[cache_id] or cache_id in self.video_exist_in_cache[video_id]:
      raise "video already exist"
    # update video_exist_in_cache
    self.cache_load_set[cache_id].add(video_id)
    # update video_exist_in_cache
    self.video_exist_in_cache[video_id].add(cache_id)

    self.cache_current_volume[cache_id] += size_videos[video_id]
    if self.cache_current_volume[cache_id] > size_cache:
      raise "cache is full"

    score_change = 0.0

    for user_id in self.cache_video_to_possible_user[cache_id][video_id]:
      user_need_videos = self.user_video_from_where[user_id]
      if video_id in user_need_videos:
        # if this user want this video
        old_cache_id = user_need_videos[video_id]
        if self.distance_cache_user[(cache_id, user_id)] < self.distance_cache_user[(old_cache_id, user_id)]:
          # if added video is closer to user

          # added video will get this new user

          # add new connection
          if user_id in self.cache_video_to_where[cache_id][video_id]:
            raise "error"
          self.cache_video_to_where[cache_id][video_id].add(user_id)

          # break old connection
          if old_cache_id != -1:
            self.cache_video_to_where[old_cache_id][video_id].remove(user_id)

          score_change += self.user_reqs[user_id][video_id] * (
            user_cache[user_id][old_cache_id] - user_cache[user_id][cache_id]
          )
          # update user_video_from_where
          self.user_video_from_where[user_id][video_id] = cache_id
    self.score += score_change * 1000.0 / total_req_count

  def del_video_from_cache(self, video_id, cache_id):
    cache_set = self.cache_load_set[cache_id]
    if video_id not in cache_set or cache_id not in self.video_exist_in_cache[video_id]:
      raise "video %d not in this cache" % video_id
    cache_set.remove(video_id)
    user_set = self.cache_video_to_where[cache_id][video_id]

    self.video_exist_in_cache[video_id].remove(cache_id)

    self.cache_current_volume[cache_id] -= size_videos[video_id]
    if self.cache_current_volume[cache_id] < 0:
      raise "cache volume < 0, impossible !"

    score_change = 0.0
    for user_id in user_set:
      all_cache_connected_to_user = set(user_cache[user_id].keys())
      all_cache_has_this_video = self.video_exist_in_cache[video_id]
      possible_cache = all_cache_connected_to_user.intersection(all_cache_has_this_video)

      dest_cache = -1
      min_time = user_cache[user_id][-1]
      if len(possible_cache) != 0:
        for cache in possible_cache:
          if user_cache[user_id][cache] < min_time:
            dest_cache = cache
            min_time = user_cache[user_id][cache]

      if dest_cache != -1:
        if user_id in self.cache_video_to_where[dest_cache][video_id]:
          raise "already there"
        self.cache_video_to_where[dest_cache][video_id].add(user_id)
      score_change += self.user_reqs[user_id][video_id] * (
        user_cache[user_id][cache_id] - user_cache[user_id][dest_cache]
      )
      self.user_video_from_where[user_id][video_id] = dest_cache

    self.cache_video_to_where[cache_id][video_id].clear()
    self.score += score_change * 1000.0 / total_req_count

  def calculate_score(self):
    save_time = 0
    total_req = 0
    for req in reqs:
      video_id, user_id, nb_req = req
      cache_id = self.user_video_from_where[user_id][video_id]
      save_time += nb_req * (user_cache[user_id][-1] - user_cache[user_id][cache_id])
      total_req += nb_req
    return 1000.0 * save_time / float(total_req)


  def fill_cache_by_best_videos(self, cache_id):
    video_list = self.cache_may_have_video[cache_id]
    video_score = [0.0 for v in video_list]
    initial_score = connections.score
    print "c=", cache_id, 'may have video=', len(video_list)
    tic = time.clock()
    for i, v in enumerate(video_list):
      # print "v=", v
      score = connections.score_changes_if_add_video_to_cache(v, cache_id)
      video_score[i] = score / float(size_videos[v])

    toc = time.clock()
    print "finding time", abs(toc - tic)
    result = sorted(zip(video_list, video_score), key=lambda x: x[1], reverse=True)
    tic = time.clock()
    print "sort time", abs(toc - tic)
    insert_count = 0
    for v_id, v_score in result:
      if v_score == 0:
        break
      rest_volume = size_cache - connections.cache_current_volume[cache_id]
      if size_videos[v_id] <= rest_volume:
        connections.add_video_to_cache(v_id, cache_id)
        insert_count += 1
    toc = time.clock()
    #print "insert time", abs(toc - tic)
    print "inserted video:", insert_count
    print "rest_volume:", size_cache - connections.cache_current_volume[cache_id]

  def empty_cache(self, cache_id):
    videos = list(self.cache_load_set[cache_id])
    for v in videos:
      self.del_video_from_cache(v, cache_id)

  def stupid_method(self):
    for c in range(num_cache):
      self.fill_cache_by_best_videos(c)
      print "score", self.score
      print "calculated score", self.calculate_score()
    for i in range(5):
      for c in range(num_cache):
        self.empty_cache(c)
        self.fill_cache_by_best_videos(c)
        print "score", self.score
        print "calculated score", self.calculate_score()



def Submission(output_file, stat_connect):
  f = open(output_file, 'w')
  f.write(str(num_cache) + '\n')
  for i in range(num_cache):
    if len(stat_connect.cache_load_set[i]) == 0:
      continue
    else:
      f.write(str(i) + ' ')
      f.write(' '.join(map(str, list(stat_connect.cache_load_set[i]))))
      f.write('\n')
  f.close()


###-- 6. Visualisation ###
# input  : a
# outpout :
file_path = "."
file_name = "videos_worth_spreading.in"
num_video, num_user, num_req, num_cache, size_cache, size_videos, user_cache, reqs, total_req_count = Read_file(
  file_path, file_name)
connections = ConnectionStatus()


connections.stupid_method()
print "score", connections.score
print "calculated score", connections.calculate_score()
Submission(file_name + ".out", connections)
