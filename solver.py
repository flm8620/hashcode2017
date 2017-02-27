import numpy as np
import os
import sys
import time
from pulp import *


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

# user_reqs[user_id] = dict{video1 : req_num1, video2 : req_num2, ...}
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


def make_unique_req():
  unique_req = {}
  for (video, endpoint, nb_request) in reqs:
    if (video, endpoint) not in unique_req:
      unique_req[(video, endpoint)] = nb_request
    else:
      unique_req[(video, endpoint)] += nb_request
  return unique_req


file_path = "."
file_name = "videos_worth_spreading.in"
num_video, num_user, num_req, num_cache, size_cache, size_videos, user_cache, reqs, total_req_count = Read_file(
  file_path, file_name)

unique_req = make_unique_req()
user_reqs = calculate_user_reqs()
prob = LpProblem("Video", LpMaximize)

target = lpSum([])
user_video_limit = [{} for i in range(num_user)]
cache_video_limit = [lpSum([]) for i in range(num_cache)]
cache_have_video = [[LpVariable('Cache_Have_Video'+str(c)+'_'+str(v),0,1,LpInteger) for v in range(num_video)] for c in range(num_cache)]
cache_have_video_inequ = []

for user_id in range(num_user):
  print "user ",user_id
  cache_dict = user_cache[user_id]
  server_delay = cache_dict[-1]
  video_req_limit = lpSum([])
  for video_id, req_number in user_reqs[user_id].iteritems():
    for cache_id, delay in cache_dict.items():
      if cache_id == -1:
        continue
      save_time = server_delay - delay

      weight = req_number * save_time / float(total_req_count) * 1000.0
      var = LpVariable("User_Cache_Video_"+str(user_id)+"_"+str(cache_id)+"_"+str(video_id),0,1,LpInteger)
      target += weight * var
      video_req_limit += var
      cache_have_video_inequ.append(cache_have_video[cache_id][video_id]>=var)
    user_video_limit[user_id][video_id]=video_req_limit

for cache_id in range(num_cache):
  print "cache", cache_id
  cache_load = lpSum([])
  for video_id in range(num_video):
    cache_load += size_videos[video_id] * cache_have_video[cache_id][video_id]
  cache_video_limit[cache_id] = cache_load

prob += target, ""
for i, video_limit in enumerate(user_video_limit):
  print "user", i
  for formular in video_limit.values():
    prob += formular <= 1, ""
for i, limit in enumerate(cache_video_limit):
  print "cache", i
  prob += limit <= size_cache, ""
for i,inequa in enumerate(cache_have_video_inequ):
  print "inequa", i
  prob += inequa, ""
prob.writeLP(file_name+".lp")
prob.solve()
print("Status:", LpStatus[prob.status])
print("Result = ", value(prob.objective))

