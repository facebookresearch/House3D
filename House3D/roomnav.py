# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.
import time
import sys, os
import numpy as np
import random
import csv
import copy
import cv2
import pickle

import gym
from gym import spaces
from .house import House
from .core import Environment, MultiHouseEnv
from . import objrender
from .objrender import RenderMode

__all__ = ['RoomNavTask']

###############################################
# Task related definitions and configurations
###############################################
flag_print_debug_info = False  # flag for printing debug info

dist_reward_scale = 1.0  # 2.0
collision_penalty_reward = 0.3  # penalty for collision
correct_move_reward = None  # reward when moving closer to target
stay_room_reward = 0.1  # reward when staying in the correct target room
indicator_reward = 0.5
success_reward = 10   # reward when success

time_penalty_reward = 0.1   # penalty for each time step
delta_reward_coef = 0.5
speed_reward_coef = 1.0

success_distance_range = 1.0
success_stay_time_steps = 5
success_see_target_time_steps = 2   # time steps required for success under the "see" criteria

# sensitivity setting
rotation_sensitivity = 30  # 45   # maximum rotation per time step
default_move_sensitivity = 0.5  # 1.0   # maximum movement per time step

# discrete action space actions, totally <13> actions
# Fwd, L, R, LF, RF, Lrot, Rrot, Bck, s-Fwd, s-L, s-R, s-Lrot, s-Rrot
discrete_actions=[(1.,0.,0.), (0.,1.,0.), (0.,-1.,0.), (0.5,0.5,0.), (0.5,-0.5,0.),
                  (0.,0.,1.), (0.,0.,-1.),
                  (-0.4,0.,0.),
                  (0.4,0.,0.), (0.,0.4,0.), (0.,-0.4,0.),
                  (0.,0.,0.4), (0.,0.,-0.4)]
n_discrete_actions = len(discrete_actions)

# criteria for seeing the object
n_pixel_for_object_see = 450   # need at least see 450 pixels for success under default resolution 120 x 90
n_pixel_for_object_sense = 50
L_pixel_reward_range = n_pixel_for_object_see - n_pixel_for_object_sense
pixel_object_reward = 0.4


#################
# Util Functions
#################

def reset_see_criteria(resolution):
    total_pixel = resolution[0] * resolution[1]
    global n_pixel_for_object_see, n_pixel_for_object_sense, L_pixel_reward_range
    n_pixel_for_object_see = max(int(total_pixel * 0.045), 5)
    n_pixel_for_object_sense = max(int(total_pixel * 0.005), 1)
    L_pixel_reward_range = n_pixel_for_object_see - n_pixel_for_object_sense


class RoomNavTask(gym.Env):
    def __init__(self, env,
                 seed=None,
                 reward_type='delta',
                 hardness=None,
                 move_sensitivity=None,
                 segment_input=True,
                 joint_visual_signal=False,
                 depth_signal=True,
                 max_steps=-1,
                 success_measure='see',
                 discrete_action=False):
        """RoomNav task wrapper with gym api
        Note:
            all the settings are the default setting to run a task
            only the <env> argument is necessary to launch the task

        Args:
            env: an instance of environment (multi-house or single-house)
            seed: if not None, set the random seed
            reward_type (str, optional): reward shaping, currently available: none, linear, indicator, delta and speed
            hardness (double, optional): if not None, must be a real number between 0 and 1, indicating the hardness
                                         namely the distance from birthplace to target (1 is the hardest)
                                         None means 1.0
            move_sensitivity (double, optional): if not None, set the maximum movement per time step (generally should not be changed)
            segment_input (bool, optional): whether to use semantic segmentation mask for observation
            joint_visual_signal (bool, optional): when true, use both visual signal and segmentation mask as observation
                                                  when true, segment_input will be set true accordingly
            depth_signal (bool, optional): whether to include depth signal in observation
            max_steps (int, optional): when max_steps > 0, the task will be cut after <max_steps> steps
            success_measure (str, optional): criteria for success, currently support 'see' and 'stay'
            discrete_action (bool, optional):  when true, use discrete actions; otherwise use continuous actions
        """
        self.env = env
        assert isinstance(env, Environment), '[RoomNavTask] env must be an instance of Environment!'
        if env.resolution != (120, 90): reset_see_criteria(env.resolution)
        self.resolution = resolution = env.resolution
        assert reward_type in [None, 'none', 'linear', 'indicator', 'delta', 'speed']
        self.reward_type = reward_type
        self.colorDataFile = self.env.config['colorFile']
        self.segment_input = segment_input
        self.joint_visual_signal = joint_visual_signal
        self.depth_signal = depth_signal
        n_channel = 3
        if segment_input:
            self.env.set_render_mode('semantic')
        else:
            self.env.set_render_mode('rgb')
        if joint_visual_signal: n_channel += 3
        if depth_signal: n_channel += 1
        self._observation_shape = (resolution[0], resolution[1], n_channel)
        self._observation_space = spaces.Box(0, 255, shape=self._observation_shape)

        self.max_steps = max_steps

        self.discrete_action = discrete_action
        if discrete_action:
            self._action_space = spaces.Discrete(n_discrete_actions)
        else:
            self._action_space = spaces.Tuple([spaces.Box(0, 1, shape=(4,)), spaces.Box(0, 1, shape=(2,))])

        if seed is not None:
            np.random.seed(seed)
            random.seed(seed)

        # configs
        self.move_sensitivity = (move_sensitivity or default_move_sensitivity)  # at most * meters per frame
        self.rot_sensitivity = rotation_sensitivity
        self.dist_scale = dist_reward_scale or 1
        self.successRew = success_reward
        self.inroomRew = stay_room_reward or 0.2
        self.colideRew = collision_penalty_reward or 0.02
        self.goodMoveRew = correct_move_reward or 0.0

        self.last_obs = None
        self.last_info = None
        self._object_cnt = 0

        # config hardness
        self.hardness = None
        self.availCoors = None
        self._availCoorsDict = None
        self.reset_hardness(hardness)

        # temp storage
        self.collision_flag = False

        # episode counter
        self.current_episode_step = 0

        # config success measure
        assert success_measure in ['stay', 'see']
        self.success_measure = success_measure
        print('[RoomNavTask] >> Success Measure = <{}>'.format(success_measure))
        self.success_stay_cnt = 0
        if success_measure == 'see':
            self.room_target_object = dict()
            self._load_target_object_data(self.env.config['roomTargetFile'])

    def _load_target_object_data(self, roomTargetFile):
        with open(roomTargetFile) as csvFile:
            reader = csv.DictReader(csvFile)
            for row in reader:
                c = np.array((row['r'],row['g'],row['b']), dtype=np.uint8)
                room = row['target_room']
                if room not in self.room_target_object:
                    self.room_target_object[room] = []
                self.room_target_object[room].append(c)

    """
    reset the target room type to navigate to
    when target is None, a valid target will be randomly selected
    """
    def reset_target(self, target=None):
        if target is None:
            target = random.choice(self.house.all_desired_roomTypes)
        else:
            assert target in self.house.all_desired_roomTypes, '[RoomNavTask] desired target <{}> does not exist in the current house!'.format(target)
        if self.house.setTargetRoom(target):  # target room changed!!!
            _id = self.house._id
            if self.house.targetRoomTp not in self._availCoorsDict[_id]:
                if self.hardness is None:
                    self.availCoors = self.house.connectedCoors
                else:
                    allowed_dist = self.house.maxConnDist * self.hardness
                    self.availCoors = [c for c in self.house.connectedCoors
                                       if self.house.connMap[c[0], c[1]] <= allowed_dist]
                self._availCoorsDict[_id][self.house.targetRoomTp] = self.availCoors
            else:
                self.availCoors = self._availCoorsDict[_id][self.house.targetRoomTp]

    @property
    def house(self):
        return self.env.house

    """
    gym api: reset function
    when target is not None, we will set the target room type to navigate to
    """
    def reset(self, target=None):
        # clear episode steps
        self.current_episode_step = 0
        self.success_stay_cnt = 0
        self._object_cnt = 0

        # reset house
        self.env.reset_house()

        self.house.targetRoomTp = None  # [NOTE] IMPORTANT! clear this!!!!!

        # reset target room
        self.reset_target(target=target)  # randomly reset

        # general birth place
        gx, gy = random.choice(self.availCoors)
        self.collision_flag = False
        # generate state
        x, y = self.house.to_coor(gx, gy, True)
        self.env.reset(x=x, y=y)
        self.last_obs = self.env.render()
        if self.joint_visual_signal:
            self.last_obs = np.concatenate([self.env.render(mode='rgb'), self.last_obs], axis=-1)
        ret_obs = self.last_obs
        if self.depth_signal:
            dep_sig = self.env.render(mode='depth')
            if dep_sig.shape[-1] > 1:
                dep_sig = dep_sig[..., 0:1]
            ret_obs = np.concatenate([ret_obs, dep_sig], axis=-1)
        self.last_info = self.info
        return ret_obs

    def _apply_action(self, action):
        if self.discrete_action:
            return discrete_actions[action]
        else:
            rot = action[1][0] - action[1][1]
            act = action[0]
            return (act[0] - act[1]), (act[2] - act[3]), rot

    def _is_success(self, raw_dist):
        if raw_dist > 0:
            self.success_stay_cnt = 0
            return False
        if self.success_measure == 'stay':
            self.success_stay_cnt += 1
            return self.success_stay_cnt >= success_stay_time_steps
        # self.success_measure == 'see'
        flag_see_target_objects = False
        object_color_list = self.room_target_object[self.house.targetRoomTp]
        if (self.last_obs is not None) and self.segment_input:
            seg_obs = self.last_obs if not self.joint_visual_signal else self.last_obs[:,:,3:6]
        else:
            seg_obs = self.env.render(mode='semantic')
        self._object_cnt = 0
        for c in object_color_list:
            cur_n = np.sum(np.all(seg_obs == c, axis=2))
            self._object_cnt += cur_n
            if self._object_cnt >= n_pixel_for_object_see:
                flag_see_target_objects = True
                break
        if flag_see_target_objects:
            self.success_stay_cnt += 1
        else:
            self.success_stay_cnt = 0  # did not see any target objects!
        return self.success_stay_cnt >= success_see_target_time_steps

    """
    gym api: step function
    return: obs, reward, done, info (a dictionary containing some state information)
    """
    def step(self, action):
        reward = 0
        det_fwd, det_hor, det_rot = self._apply_action(action)
        move_fwd = det_fwd * self.move_sensitivity
        move_hor = det_hor * self.move_sensitivity
        rotation = det_rot * self.rot_sensitivity

        self.env.rotate(rotation)
        if not self.env.move_forward(move_fwd, move_hor):
            if flag_print_debug_info:
                print('Collision! No Movement Performed!')
            reward -= self.colideRew
            self.collision_flag = True
        else:
            self.collision_flag = False
            if flag_print_debug_info:
                print('Move Successfully!')

        self.last_obs = obs = self.env.render()
        if self.joint_visual_signal:
            self.last_obs = obs = np.concatenate([self.env.render(mode='rgb'), obs], axis=-1)
        cur_info = self.info
        raw_dist = cur_info['dist']
        orig_raw_dist = self.last_info['dist']
        if raw_dist < orig_raw_dist:
            reward += self.goodMoveRew
        if raw_dist == 0:
            reward += self.inroomRew
        raw_scaled_dist = cur_info['scaled_dist']
        dist = raw_scaled_dist * self.dist_scale
        done = False
        if self._is_success(raw_dist):
            reward += self.successRew
            done = True
        # accumulate episode step
        self.current_episode_step += 1
        if (self.max_steps > 0) and (self.current_episode_step >= self.max_steps): done = True

        # reward shaping
        if self.reward_type == 'linear':
            reward -= dist
        elif self.reward_type == 'indicator':
            if raw_dist != orig_raw_dist:  # indicator reward
                reward += indicator_reward if raw_dist < orig_raw_dist else -indicator_reward
            if raw_dist >= orig_raw_dist: reward -= time_penalty_reward
        elif self.reward_type == 'delta':
            delta_raw_dist = orig_raw_dist - raw_dist
            ratio = self.move_sensitivity / self.house.grid_det
            delta_reward = delta_raw_dist / ratio * delta_reward_coef
            delta_reward = np.clip(delta_reward, -indicator_reward, indicator_reward)
            reward += delta_reward
            if raw_dist >= orig_raw_dist: reward -= time_penalty_reward
        elif self.reward_type == 'speed':
            movement = np.sqrt((self.last_info['pos'][0]-cur_info['pos'][0])**2
                               + (self.last_info['pos'][1]-cur_info['pos'][1])**2)
            sign = np.sign(orig_raw_dist - raw_dist)
            det_dist = movement * sign * speed_reward_coef
            det_dist = np.clip(det_dist, -indicator_reward, indicator_reward)
            reward += det_dist
            if raw_dist >= orig_raw_dist: reward -= time_penalty_reward

        # object seen reward
        if (raw_dist == 0) and (self.success_measure == 'see'):  # inside target room and success measure is <see>
            if not done:
                object_reward = np.clip((self._object_cnt - n_pixel_for_object_sense) / L_pixel_reward_range, 0., 1.) * pixel_object_reward
                reward += object_reward

        if self.depth_signal:
            dep_sig = self.env.render(mode='depth')
            if dep_sig.shape[-1] > 1:
                dep_sig = dep_sig[..., 0:1]
            obs = np.concatenate([obs, dep_sig], axis=-1)
        self.last_info = cur_info
        return obs, reward, done, cur_info

    @property
    def observation_space(self):
        return self._observation_space

    @property
    def action_space(self):
        return self._action_space

    @property
    def info(self):
        ret = self.env.info
        gx, gy = ret['grid']
        ret['dist'] = dist = self.house.connMap[gx, gy]
        ret['scaled_dist'] = self.house.getScaledDist(gx, gy)
        ret['optsteps'] = int(dist / (self.move_sensitivity / self.house.grid_det) + 0.5)
        ret['collision'] = int(self.collision_flag)
        ret['target_room'] = self.house.targetRoomTp
        return ret

    """
    return all the available target room types of the current house
    """
    def get_avail_targets(self):
        return self.house.all_desired_roomTypes

    """
    reset the hardness of the task
    """
    def reset_hardness(self, hardness=None):
        self.hardness = hardness
        if hardness is None:
            self.availCoors = self.house.connectedCoors
        else:
            allowed_dist = self.house.maxConnDist * hardness
            self.availCoors = [c for c in self.house.connectedCoors
                               if self.house.connMap[c[0], c[1]] <= allowed_dist]
        n_house = self.env.num_house
        self._availCoorsDict = [dict() for i in range(n_house)]
        self._availCoorsDict[self.house._id][self.house.targetRoomTp] = self.availCoors

    """
    recover the state (location) of the agent from the info dictionary
    """
    def set_state(self, info):
        self.env.reset(x=info['pos'][0], y=info['pos'][1], yaw=info['yaw'])

    """
    return 2d topdown map
    """
    def get_2dmap(self):
        return self.env.gen_2dmap()

    """
    show a image.
    if img is not None, it will visualize the img to monitor
    if img is None, it will return a img object of the observation
    Note: whenever this function is called with img not None, the task cannot perform rendering any more!!!!
    """
    def show(self, img=None):
        return self.env.show(img=img,
                             renderMapLoc=(None if img is not None else self.info['grid']),
                             renderSegment=True,
                             display=(img is not None))

    def debug_show(self):
        return self.env.debug_render()

if __name__ == '__main__':
    from .common import load_config

    api = objrender.RenderAPI(
            w=400, h=300, device=0)
    cfg = load_config('config.json')

    houses = ['00065ecbdd7300d35ef4328ffe871505',
    'cf57359cd8603c3d9149445fb4040d90', '31966fdc9f9c87862989fae8ae906295', 'ff32675f2527275171555259b4a1b3c3',
    '7995c2a93311717a3a9c48d789563590', '8b8c1994f3286bfc444a7527ffacde86', '775941abe94306edc1b5820e3a992d75',
    '32e53679b33adfcc5a5660b8c758cc96', '4383029c98c14177640267bd34ad2f3c', '0884337c703e7c25949d3a237101f060',
    '492c5839f8a534a673c92912aedc7b63', 'a7e248efcdb6040c92ac0cdc3b2351a6', '2364b7dcc432c6d6dcc59dba617b5f4b',
    'e3ae3f7b32cf99b29d3c8681ec3be321', 'f10ce4008da194626f38f937fb9c1a03', 'e6f24af5f87558d31db17b86fe269cf2',
    '1dba3a1039c6ec1a3c141a1cb0ad0757', 'b814705bc93d428507a516b866efda28', '26e33980e4b4345587d6278460746ec4',
    '5f3f959c7b3e6f091898caa8e828f110', 'b5bd72478fce2a2dbd1beb1baca48abd', '9be4c7bee6c0ba81936ab0e757ab3d61']
    #env = MultiHouseEnv(api, houses[:3], cfg)  # use 3 houses
    env = Environment(api, houses[0], cfg)
    task = RoomNavTask(env, hardness=0.6, discrete_action=True)

    action_dict = dict(k=7,l=9,j=10,o=3,u=4,f=5,a=6,i=8,d=11,s=12,r=-1,h=-2,q=-3)
    def print_help():
        print('Usage: ')
        print('> Actions: (Simplified Version) Total 10 Actions')
        print('  --> j, k, l, i, u, o: left, back, right, forward, left-forward, right-forward')
        print('  --> a, s, d, f: left-rotate, small left-rot, small right-rot, right-rotate')
        print('> press r: reset')
        print('> press h: show helper again')
        print('> press q: exit')

    print_help()
    eval = []
    while True:
        step = 0
        rew = 0
        good = 0
        task.reset()
        target = task.info['target_room']
        while True:
            print('Step#%d, Instruction = <go to %s>' % (step, target))
            mat = task.debug_show()
            cv2.imshow("aaa", mat)
            while True:
                key = cv2.waitKey(0)
                key = chr(key)
                if key in action_dict:
                    if key == 'h':
                        print_help()
                    else:
                        break
                else:
                    print('>> invalid key! press q to quit; r to reset; h to get helper')
            if action_dict[key] < 0:
                break
            step += 1
            obs, reward, done, info = task.step(action_dict[key])
            rew += reward
            print('>> r = %.2f, done = %f, accu_rew = %.2f, step = %d' % (reward, done, rew, step))
            print('   info: collision = %d, raw_dist = %d, scaled_dist = %.3f' % (info['collision'], info['dist'], info['scaled_dist']))
            if done:
                good = 1
                print('Congratulations! You reach the Target!')
                print('>> Press any key to restart!')
                key = cv2.waitKey(0)
                break
        eval.append((step, rew, good))
        if key == 'q':
            break
    if len(eval) > 0:
        print('++++++++++ Task Stats +++++++++++')
        print("Episode Played: %d" % len(eval))
        succ = [e for e in eval if e[2] > 0]
        print("Success = %d, Rate = %.3f" % (len(succ), len(succ) / len(eval)))
        print("Avg Reward = %.3f" % (sum([e[1] for e in eval])/len(eval)))
        if len(succ) > 0:
            print("Avg Success Reward = %.3f" % (sum([e[1] for e in succ]) / len(succ)))
        print("Avg Step = %.3f" % (sum([e[0] for e in eval]) / len(eval)))
        if len(succ) > 0:
            print("Avg Success Step = %.3f" % (sum([e[0] for e in succ]) / len(succ)))
