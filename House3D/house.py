# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import csv
import cv2
import itertools
import json
import numpy as np
import pickle
import time

__all__ = ['House']

######################################
# Util Functions
######################################
# allowed target room types
# NOTE: consider "toilet" and "bathroom" the same thing
ALLOWED_TARGET_ROOM_TYPES = ['kitchen', 'dining_room', 'living_room', 'bathroom', 'bedroom']  # 'office'

# allowed room types for auxiliary prediction task
ALLOWED_PREDICTION_ROOM_TYPES = dict(
    outdoor=0, indoor=1, kitchen=2, dining_room=3, living_room=4, bathroom=5, bedroom=6, office=7, storage=8)

def _equal_room_tp(room, target):
    """
    NOTE: Ensure <target> is always from <ALLOWED_TARGET_ROOM_TYPES>!!!!
            DO NOT swap the order of arguments
    """
    room = room.lower()
    target = target.lower()
    return (room == target) or \
            ((target == 'bathroom') and (room == 'toilet')) or \
            ((target == 'bedroom') and (room == 'guest_room'))

def _get_pred_room_tp_id(room):
    room = room.lower()
    if room == 'toilet':
        room = 'bathroom'
    elif room == 'guest_room':
        room = 'bedroom'
    if room not in ALLOWED_PREDICTION_ROOM_TYPES:
        return ALLOWED_PREDICTION_ROOM_TYPES['indoor']
    return ALLOWED_PREDICTION_ROOM_TYPES[room]


def parse_walls(objFile, lower_bound = 1.0):
    def create_box(vers):
        if len(vers) == 0:
            return None
        v_max = [-1e20, -1e20, -1e20]
        v_min = [1e20, 1e20, 1e20]
        for v in vers:
            for i in range(3):
                if v[i] < v_min[i]: v_min[i] = v[i]
                if v[i] > v_max[i]: v_max[i] = v[i]
        obj = {}
        obj['bbox'] = {}
        obj['bbox']['min']=v_min
        obj['bbox']['max']=v_max
        if v_min[1] < lower_bound:
            return obj
        return None
    walls = []
    with open(objFile, 'r') as file:
        vers = []
        for line in file.readlines():
            if len(line) < 2: continue
            if line[0] == 'g':
                if (vers is not None) and (len(vers) > 0): walls.append(create_box(vers))
                if ('Wall' in line):
                    vers = []
                else:
                    vers = None
            if (vers is not None) and (line[0] == 'v') and (line[1] == ' '):
                vals = line[2:]
                coor =[float(v) for v in vals.split(' ') if len(v)>0]
                if len(coor) != 3:
                    print('line = {}'.format(line))
                    print('coor = {}'.format(coor))
                    assert(False)
                vers.append(coor)
    if (vers is not None) and (len(vers) > 0): walls.append(create_box(vers))
    ret_walls = [w for w in walls if w is not None]
    return ret_walls


def fill_region(proj, x1, y1, x2, y2, c):
    proj[x1:(x2 + 1), y1:(y2 + 1)] = c


def fill_obj_mask(house, dest, obj, c=1):
    n_row = dest.shape[0]
    _x1, _, _y1 = obj['bbox']['min']
    _x2, _, _y2 = obj['bbox']['max']
    x1,y1,x2,y2 = house.rescale(_x1,_y1,_x2,_y2,n_row)
    fill_region(dest, x1, y1, x2, y2, c)


class House(object):
    """core class for loading and processing a house from SUNCG dataset
    """
    def __init__(self, JsonFile, ObjFile, MetaDataFile,
                 CachedFile=None,
                 StorageFile=None,
                 GenRoomTypeMap=False,
                 EagleViewRes=100,
                 DebugInfoOn=False,
                 ColideRes=1000,
                 RobotRadius=0.1,
                 RobotHeight=0.75,  # 1.0,
                 CarpetHeight=0.15,
                 SetTarget=True,
                 ApproximateMovableMap=False,
                 _IgnoreSmallHouse=False,  # should be only set true when called by "cache_houses.py"
                 DebugMessages=True
                 ):
        """Initialization and Robot Parameters

         Note:
            Generally only the first 4 arguments are required to set up a house
            Ensure you run the script to generate cached data for all the houses

        Args:
            JsonFile (str): file name of the house json file (house.json)
            ObjFile (str): file name of the house object file (house.obj)
            MetaDataFile (str): file name of the meta data (ModelCategoryMapping.csv)
            CachedFile (str, recommended): file name of the pickled cached data for this house, None if no such cache (cachedmap1k.pkl)
            StorageFile (str, optional): if CachedFile is None, pickle all the data and store in this file
            GenRoomTypeMap (bool, optional): if turned on, generate the room type map for each location
            EagleViewRes (int, optional): resolution of the topdown 2d map
            DebugInfoOn (bool, optional): store additional debugging information when this option is on
            ColideRes (int, optional): resolution of the 2d map for collision check (generally should not changed)
            RobotRadius (double, optional): radius of the robot/agent (generally should not be changed)
            RobotHeight (double, optional): height of the robot/agent (generally should not be changed)
            CarpetHeight (double, optional): maximum height of the obstacles that agent can directly go through (gennerally should not be changed)
            SetTarget (bool, optional): whether or not to choose a default target room and pre-compute the valid locations
            ApproximateMovableMap (bool, optional): Fast initialization of valid locations which are not as accurate or fine-grained.  Requires OpenCV if true
            DebugMessages=True (bool, optional): whether or not to show debug messages
        """
        if DebugMessages == True:
            ts = time.time()
            print('Data Loading ...')

        self.metaDataFile = MetaDataFile
        self.objFile = ObjFile
        self.robotHei = RobotHeight
        self.carpetHei = CarpetHeight
        self.robotRad = RobotRadius
        self._debugMap = None if not DebugInfoOn else True
        with open(JsonFile) as jfile:
            self.house = house = json.load(jfile)
        self.all_walls = parse_walls(ObjFile, RobotHeight)

        # validity check
        if abs(house['scaleToMeters'] - 1.0) > 1e-8:
            print('[Error] Currently <scaleToMeters> must be 1.0!')
            assert(False)
        if len(house['levels']) > 1 and DebugMessages == True:
            print('[Warning] Currently only support ground floor! <total floors = %d>' % (len(house['levels'])))

        self.level = level = house['levels'][0]  # only support ground floor now
        self.L_min_coor = _L_lo = np.array(level['bbox']['min'])
        self.L_lo = min(_L_lo[0], _L_lo[2])
        self.L_max_coor = _L_hi = np.array(level['bbox']['max'])
        self.L_hi = max(_L_hi[0], _L_hi[2])
        self.L_det = self.L_hi - self.L_lo
        self.n_row = ColideRes
        self.eagle_n_row = EagleViewRes
        self.grid_det = self.L_det / self.n_row
        self.all_obj = [node for node in level['nodes'] if node['type'].lower() == 'object']
        self.all_rooms = [node for node in level['nodes'] if (node['type'].lower() == 'room') and ('roomTypes' in node)]
        self.all_roomTypes = [room['roomTypes'] for room in self.all_rooms]
        self.all_desired_roomTypes = []
        self.default_roomTp = None
        for roomTp in ALLOWED_TARGET_ROOM_TYPES:
            if any([any([_equal_room_tp(tp, roomTp) for tp in tps]) for tps in self.all_roomTypes]):
                self.all_desired_roomTypes.append(roomTp)
                if self.default_roomTp is None: self.default_roomTp = roomTp
        assert self.default_roomTp is not None, 'Cannot Find Any Desired Rooms!'

        if DebugMessages == True:
            print('>> Default Target Room Type Selected = {}'.format(self.default_roomTp))
            print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))

        if _IgnoreSmallHouse and ((len(self.all_desired_roomTypes) < 2) or ('kitchen' not in self.all_desired_roomTypes)):
            self.all_desired_roomTypes=[]
            return

        if DebugMessages == True:
            print('Generating Low Resolution Obstacle Map ...')
            ts = time.time()
        # generate a low-resolution obstacle map
        self.tinyObsMap = np.ones((self.eagle_n_row, self.eagle_n_row), dtype=np.uint8)
        self.genObstacleMap(MetaDataFile, gen_debug_map=False, dest=self.tinyObsMap, n_row=self.eagle_n_row-1)
        self.eagleMap = np.zeros((4, self.eagle_n_row, self.eagle_n_row), dtype=np.uint8)
        self.eagleMap[0, ...] = self.tinyObsMap

        if DebugMessages == True:
            print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))

        # load from cache
        if CachedFile is not None:
            assert not DebugInfoOn, 'Please set DebugInfoOn=True when loading data from cached file!'

            if DebugMessages == True:
                print('Loading Obstacle Map and Movability Map From Cache File ...')
                ts = time.time()
            with open(CachedFile, 'rb') as f:
                self.obsMap, self.moveMap = pickle.load(f)

            if DebugMessages == True:
                print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))
        else:
            # generate obstacle map
            if DebugMessages == True:
                print('Generate High Resolution Obstacle Map (For Collision Check) ...')
                ts = time.time()
            # obsMap was indexed by (x, y), not (y, x)
            self.obsMap = np.ones((self.n_row+1, self.n_row+1), dtype=np.uint8)  # a small int is enough
            if self._debugMap is not None:
                self._debugMap = np.ones((self.n_row+1, self.n_row+1), dtype=np.float)
            self.genObstacleMap(MetaDataFile)
            if DebugMessages == True:
                print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))

            # generate movability map for robots considering the radius
            if DebugMessages == True:
                print('Generate Movability Map ...')
                ts = time.time()
            self.moveMap = np.zeros((self.n_row+1, self.n_row+1), dtype=np.int8)  # initially not movable
            self.genMovableMap(ApproximateMovableMap)
            if DebugMessages == True:
                print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))

            if StorageFile is not None:
                if DebugMessages == True:
                    print('Storing Obstacle Map and Movability Map to Cache File ...')
                    ts = time.time()
                with open(StorageFile, 'wb') as f:
                    pickle.dump([self.obsMap, self.moveMap], f)
                if DebugMessages == True:
                    print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))

        # set target room connectivity
        if DebugMessages == True:
            ts = time.time()
        self.connMapDict = {}
        self.roomTypeLocMap = {}    # roomType -> feasible locations
        self.targetRoomTp = None
        self.targetRooms = []
        self.connMap = None
        self.inroomDist = None
        if SetTarget:
            if DebugMessages == True:
                print('Generate Target connectivity Map (Default <{}>) ...'.format(self.default_roomTp))
            self.setTargetRoom(self.default_roomTp, _setEagleMap=True)
        if DebugMessages == True:
            print('  --> Done! Elapsed = %.2fs' % (time.time()-ts))

        self.roomTypeMap = None
        if GenRoomTypeMap:
            if DebugMessages == True:
                ts = time.time()
                print('Generate Room Type Map ...')
            self.roomTypeMap = np.zeros((self.n_row+1, self.n_row+1), dtype=np.uint16)
            self._generate_room_type_map()
            if DebugMessages == True:
                print('  --> Done! Elapsed = %.2fs' % (time.time() - ts))


    def _generate_room_type_map(self):
        rtMap = self.roomTypeMap
        # fill all the mask of rooms
        for room in self.all_rooms:
            msk = 1 << _get_pred_room_tp_id('indoor')
            for tp in room['roomTypes']: msk = msk | (1 << _get_pred_room_tp_id(tp))
            _x1, _, _y1 = room['bbox']['min']
            _x2, _, _y2 = room['bbox']['max']
            x1, y1, x2, y2 = self.rescale(_x1, _y1, _x2, _y2)
            for x in range(x1, x2+1):
                for y in range(y1, y2+1):
                    if self.moveMap[x, y] > 0:
                        rtMap[x, y] = rtMap[x, y] | msk
        for x in range(self.n_row+1):
            for y in range(self.n_row+1):
                if (self.moveMap[x, y] > 0) and (rtMap[x, y] == 0):
                    rtMap[x, y] = 1 << _get_pred_room_tp_id('outdoor')


    def _find_components(self, x1, y1, x2, y2, dirs=None, return_largest=False, return_open=False):
        """
        return a list of components (coors), which are grid locations connencted and canMove
        @:param return_largest: return_largest == True, return only a single list of coors, the largest components
        @:param return_open: return_open == True, return only those components connected to outside of the room
        @:param dirs: connected directions, by default 4-connected (L,R,U,D)
        """
        if dirs is None:
            dirs = [[0, 1], [1, 0], [-1, 0], [0, -1]]
        comps = []
        open_comps = set()
        visit = {}
        n = 0
        for x in range(x1, x2+1):
            for y in range(y1, y2+1):
                pos = (x, y)
                if self.canMove(x, y) and (pos not in visit):
                    que = [pos]
                    visit[pos] = n
                    ptr = 0
                    is_open = False
                    while ptr < len(que):
                        cx, cy = que[ptr]
                        ptr += 1
                        for det in dirs:
                            tx, ty = cx + det[0], cy + det[1]
                            if self.canMove(tx, ty):
                                if (tx < x1) or (tx > x2) or (ty < y1) or (ty > y2):
                                    is_open=True
                                    continue
                                tp = (tx, ty)
                                if tp not in visit:
                                    visit[tp] = n
                                    que.append(tp)
                    if is_open: open_comps.add(n)
                    n += 1
                    comps.append(que)
        if n == 0: return []  # no components found!
        ret_comps = comps
        if return_open:
            if len(open_comps) == 0:
                print('WARNING!!!! [House] <find components in Target Room [%s]> No Open Components Found!!!! Return Largest Instead!!!!' % self.targetRoomTp)
                return_largest = True
            else:
                ids = sorted(list(open_comps))
                ret_comps = [comps[i] for i in ids]
        if return_largest:
            max_c = np.argmax([len(c) for c in ret_comps])
            ret_comps = ret_comps[max_c]
        # del visit
        return ret_comps

    """
    Sets self.connMap to distances to target point with some margin
    """
    def setTargetPoint(self, x, y, margin_x=15, margin_y=15):
        self.connMap = connMap = np.ones((self.n_row+1, self.n_row+1), dtype=np.int32) * -1
        self.inroomDist = inroomDist = np.ones((self.n_row+1, self.n_row+1), dtype=np.float32) * -1
        dirs = [[0, 1], [1, 0], [-1, 0], [0, -1]]

        x1, y1, x2, y2 = x-margin_x, y-margin_y, x+margin_x, y+margin_y
        _x, _y = self.to_coor(x, y)

        curr_components = self._find_components(x1, y1, x2, y2, dirs=dirs, return_open=True)
        if len(curr_components) == 0:
            return False

        que = []
        if isinstance(curr_components[0], list):  # join all the coors in the open components
            curr_major_coors = list(itertools.chain(*curr_components))
        else:
            curr_major_coors = curr_components
        min_dist_to_center = 1e50
        for xx, yy in curr_major_coors:
            connMap[xx, yy] = 0
            que.append((xx, yy))
            tx, ty = self.to_coor(xx, yy)
            tdist = np.sqrt((tx - _x) ** 2 + (ty - _y) ** 2)
            if tdist < min_dist_to_center:
                min_dist_to_center = tdist
            inroomDist[xx, yy] = tdist
        for xx, yy in curr_major_coors:
            inroomDist[xx, yy] -= min_dist_to_center

        ptr = 0
        self.maxConnDist = 1
        while ptr < len(que):
            xx, yy = que[ptr]
            cur_dist = connMap[xx, yy]
            ptr += 1
            for dx, dy in dirs:
                tx, ty = xx+dx, yy+dy
                if self.inside(tx,ty) and self.canMove(tx,ty) and not self.isConnect(tx, ty):
                    que.append((tx,ty))
                    connMap[tx,ty] = cur_dist + 1
                    if cur_dist + 1 > self.maxConnDist:
                        self.maxConnDist = cur_dist + 1

        return True

    """
    set the distance to a particular room type
    """
    def setTargetRoom(self, targetRoomTp = 'kitchen', _setEagleMap = False):
        targetRoomTp = targetRoomTp.lower()
        assert targetRoomTp in ALLOWED_TARGET_ROOM_TYPES, '[House] room type <{}> not supported!'.format(targetRoomTp)
        if targetRoomTp == self.targetRoomTp:
            return False  # room not changed!
        else:
            self.targetRoomTp = targetRoomTp
        ###########
        # Caching
        if targetRoomTp in self.connMapDict:
            self.connMap, self.connectedCoors, self.inroomDist, self.maxConnDist = self.connMapDict[targetRoomTp]
            return True  # room Changed!
        self.targetRooms = targetRooms = \
            [room for room in self.all_rooms if any([ _equal_room_tp(tp, targetRoomTp) for tp in room['roomTypes']])]
        assert (len(targetRooms) > 0), '[House] no room of type <{}> in the current house!'.format(targetRoomTp)
        ##########
        # generate destination mask map
        if _setEagleMap:  # TODO: Currently a hack to speedup mult-target learning!!! So eagleMap become *WRONG*!
            self.eagleMap[1, ...] = 0
            for room in self.targetRooms:
                _x1, _, _y1 = room['bbox']['min']
                _x2, _, _y2 = room['bbox']['max']
                x1,y1,x2,y2 = self.rescale(_x1,_y1,_x2,_y2,self.eagleMap.shape[1]-1)
                self.eagleMap[1, x1:(x2+1), y1:(y2+1)]=1
        print('[House] Caching New ConnMap for Target <{}>! (total {} rooms involved)'.format(targetRoomTp,len(targetRooms)))
        self.connMap = connMap = np.ones((self.n_row+1, self.n_row+1), dtype=np.int32) * -1
        self.inroomDist = inroomDist = np.ones((self.n_row+1, self.n_row+1), dtype=np.float32) * -1
        dirs = [[0, 1], [1, 0], [-1, 0], [0, -1]]
        que = []
        for flag_find_open_components in [True, False]:
            if not flag_find_open_components:
                print('WARINING!!!! [House] No Space Found for Room Type {}! Now search even for closed region!!!'.format(targetRoomTp))
            for room in targetRooms:
                _x1, _, _y1 = room['bbox']['min']
                _x2, _, _y2 = room['bbox']['max']
                cx, cy = (_x1 + _x2) / 2, (_y1 + _y2) / 2
                x1,y1,x2,y2 = self.rescale(_x1,_y1,_x2,_y2)
                curr_components = self._find_components(x1, y1, x2, y2, dirs=dirs, return_open=flag_find_open_components)  # find all the open components
                if len(curr_components) == 0:
                    print('WARNING!!!! [House] No Space Found in TargetRoom <tp=%s, bbox=[%.2f, %2f] x [%.2f, %.2f]>' %
                          (targetRoomTp, _x1, _x2, _y1, _y2))
                    continue
                if isinstance(curr_components[0], list):  # join all the coors in the open components
                    curr_major_coors = list(itertools.chain(*curr_components))
                else:
                    curr_major_coors = curr_components
                min_dist_to_center = 1e50
                for x, y in curr_major_coors:
                    connMap[x, y] = 0
                    que.append((x, y))
                    tx, ty = self.to_coor(x, y)
                    tdist = np.sqrt((tx - cx) ** 2 + (ty - cy) ** 2)
                    if tdist < min_dist_to_center:
                        min_dist_to_center = tdist
                    inroomDist[x, y] = tdist
                for x, y in curr_major_coors:
                    inroomDist[x, y] -= min_dist_to_center
            if len(que) > 0:
                break
        assert len(que) > 0, "Error!! [House] No space found for room type {}. House ID = {}"\
            .format(targetRoomTp, (self._id if hasattr(self, '_id') else 'NA'))
        ptr = 0
        self.maxConnDist = 1
        while ptr < len(que):
            x,y = que[ptr]
            cur_dist = connMap[x, y]
            ptr += 1
            for dx,dy in dirs:
                tx,ty = x+dx,y+dy
                if self.inside(tx,ty) and self.canMove(tx,ty) and not self.isConnect(tx, ty):
                    que.append((tx,ty))
                    connMap[tx,ty] = cur_dist + 1
                    if cur_dist + 1 > self.maxConnDist:
                        self.maxConnDist = cur_dist + 1
        self.connMapDict[targetRoomTp] = (connMap, que, inroomDist, self.maxConnDist)
        self.connectedCoors = que
        print(' >>>> ConnMap Cached!')
        return True  # room changed!


    def _getRoomBounds(self, room):
        _x1, _, _y1 = room['bbox']['min']
        _x2, _, _y2 = room['bbox']['max']
        return self.rescale(_x1, _y1, _x2, _y2)

    """
    returns a random location of a given room type
    """
    def getRandomLocation(self, roomTp):
        roomTp = roomTp.lower()
        assert roomTp in ALLOWED_TARGET_ROOM_TYPES, '[House] room type <{}> not supported!'.format(roomTp)
        # get list of valid locations within the room bounds
        locations = []
        rooms = self._getRooms(roomTp)
        for room in rooms:
            room_locs = self._getValidRoomLocations(room)
            if room_locs and len(room_locs) > 0:
                locations.extend(room_locs)

        # choose random location
        result = None
        if len(locations) > 0:
            idx = np.random.choice(len(locations))
            result = self.to_coor(locations[idx][0], locations[idx][1], True)

        return result


    def getRandomLocationForRoom(self, room_node):
        '''Given a room node from the SUNCG house.json, returns a randomly
           sampled valid location from that room.  Returns None if no valid
           locations are found.
        '''
        room_locs = self._getValidRoomLocations(room_node)
        if len(room_locs) == 0:
            return None
        idx = np.random.choice(len(room_locs))
        return self.to_coor(room_locs[idx][0], room_locs[idx][1], True)


    def _getValidRoomLocations(self, room_node):
        room_locs = None
        if room_node['id'] in self.roomTypeLocMap:
            room_locs = self.roomTypeLocMap[room_node['id']]
        else:
            room_bounds = self._getRoomBounds(room_node)
            room_locs = self._find_components(*room_bounds, return_largest=True)
            self.roomTypeLocMap[room_node['id']] = room_locs
        return room_locs


    """
    cache the shortest distance to all the possible room types
    """
    def cache_all_target(self):
        for t in self.all_desired_roomTypes:
            self.setTargetRoom(t)
        self.setTargetRoom(self.default_roomTp)

    def genObstacleMap(self, MetaDataFile, gen_debug_map=True, dest=None, n_row=None):
        # load all the doors
        target_match_class = 'nyuv2_40class'
        target_door_labels = ['door', 'fence', 'arch']
        door_ids = set()
        fine_grained_class = 'fine_grained_class'
        ignored_labels = ['person', 'umbrella', 'curtain']
        person_ids = set()
        window_ids = set()
        with open(MetaDataFile) as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                if row[target_match_class] in target_door_labels:
                    door_ids.add(row['model_id'])
                if row[target_match_class] == 'window':
                    window_ids.add(row['model_id'])
                if row[fine_grained_class] in ignored_labels:
                    person_ids.add(row['model_id'])
        def is_door(obj):
            if obj['modelId'] in door_ids:
                return True
            if (obj['modelId'] in window_ids) and (obj['bbox']['min'][1] < self.carpetHei):
                return True
            return False

        solid_obj = [obj for obj in self.all_obj if (not is_door(obj)) and (obj['modelId'] not in person_ids)]  # ignore person
        door_obj = [obj for obj in self.all_obj if is_door(obj)]
        colide_obj = [obj for obj in solid_obj if obj['bbox']['min'][1] < self.robotHei and obj['bbox']['max'][1] > self.carpetHei]
        # generate the map for all the obstacles
        obsMap = dest if dest is not None else self.obsMap
        if n_row is None:
            n_row = obsMap.shape[0] - 1
        x1,y1,x2,y2 = self.rescale(self.L_min_coor[0],self.L_min_coor[2],self.L_max_coor[0],self.L_max_coor[2],n_row)  # fill the space of the level
        fill_region(obsMap,x1,y1,x2,y2,0)
        if gen_debug_map and (self._debugMap is not None):
            fill_region(self._debugMap, x1, y1, x2, y2, 0)
        # fill boundary of rooms
        maskRoom = np.zeros_like(obsMap,dtype=np.int8)
        for wall in self.all_walls:
            _x1, _, _y1 = wall['bbox']['min']
            _x2, _, _y2 = wall['bbox']['max']
            x1,y1,x2,y2 = self.rescale(_x1,_y1,_x2,_y2,n_row)
            fill_region(obsMap, x1, y1, x2, y2, 1)
            if gen_debug_map and (self._debugMap is not None):
                fill_region(self._debugMap, x1, y1, x2, y2, 1)
            fill_region(maskRoom, x1, y1, x2, y2, 1)
        # remove all the doors
        for obj in door_obj:
            _x1, _, _y1 = obj['bbox']['min']
            _x2, _, _y2 = obj['bbox']['max']
            x1,y1,x2,y2 = self.rescale(_x1,_y1,_x2,_y2,n_row)
            cx = (x1 + x2) // 2
            cy = (y1 + y2) // 2
            # expand region
            if x2 - x1 < y2 - y1:
                while (x1 - 1 >= 0) and (maskRoom[x1-1,cy] > 0):
                    x1 -= 1
                while (x2 + 1 < maskRoom.shape[0]) and (maskRoom[x2+1,cy] > 0):
                    x2 += 1
            else:
                while (y1 - 1 >= 0) and (maskRoom[cx,y1-1] > 0):
                    y1 -= 1
                while (y2+1 < maskRoom.shape[1]) and (maskRoom[cx,y2+1] > 0):
                    y2 += 1
            fill_region(obsMap,x1,y1,x2,y2,0)
            if gen_debug_map and (self._debugMap is not None):
                fill_region(self._debugMap, x1, y1, x2, y2, 0.5)

        # mark all the objects obstacle
        for obj in colide_obj:
            _x1, _, _y1 = obj['bbox']['min']
            _x2, _, _y2 = obj['bbox']['max']
            x1,y1,x2,y2 = self.rescale(_x1,_y1,_x2,_y2,n_row)
            fill_region(obsMap,x1,y1,x2,y2,1)
            if gen_debug_map and (self._debugMap is not None):
                fill_region(self._debugMap, x1, y1, x2, y2, 0.8)



    def genMovableMap(self, approximate=False):
        roi_bounds = self._getRegionsOfInterest()
        for roi in roi_bounds:
            if approximate:
                self._updateMovableMapApproximate(*roi)
            else:
                self._updateMovableMap(*roi)

        if approximate:
            self._adjustApproximateRobotMoveMap()


    def _adjustApproximateRobotMoveMap(self):
        # Here we haven't yet accounted for the robot radius, so do some
        # approximate accommodation
        robotGridSize = int(np.rint(self.robotRad * 2 * self.n_row / self.L_det))
        if robotGridSize > 1:
            robotGridRadius = robotGridSize // 2
            kernel = np.zeros((robotGridSize, robotGridSize), np.uint8)
            cv2.circle(kernel, (robotGridRadius + 1, robotGridRadius + 1), robotGridRadius, color=1, thickness=-1)
            filtered_obstacles = (self.moveMap == 0).astype(np.uint8)
            dilated_obstacles = cv2.dilate(filtered_obstacles, kernel, iterations=1)
            self.moveMap = (dilated_obstacles == 0).astype(np.uint8)


    def _updateMovableMap(self, x1, y1, x2, y2):
        for i in range(x1, x2):
            for j in range(y1, y2):
                if self.obsMap[i,j] == 0:
                    cx, cy = self.to_coor(i, j, True)
                    if self.check_occupy(cx,cy):
                        self.moveMap[i,j] = 1


    def _updateMovableMapApproximate(self, x1, y1, x2, y2):
        self.moveMap[x1:x2, y1:y2] = (self.obsMap[x1:x2, y1:y2] == 0).astype(self.moveMap.dtype)


    def _getRegionsOfInterest(self):
        """Override this function for customizing the areas of the map to
        consider when marking valid movable locations
        Returns a list of (x1, y1, x2, y2) tuples representing bounding boxes
        of valid areas.  Coordinates are normalized grid coordinates.
        """
        return [(0, 0, self.n_row+1, self.n_row+1)]


    """
    check whether the *grid* coordinate (x,y) is inside the house
    """
    def inside(self,x,y):
        return x >= 0 and y >=0 and x <= self.n_row and y <= self.n_row

    """
    get the corresponding grid coordinate of (x, y) in the topdown 2d map
    """
    def get_eagle_view_grid(self, x, y, input_grid=False):
        if input_grid:
            x, y = self.to_coor(x, y, shft=True)
        return self.to_grid(x, y, n_row=self.eagle_n_row-1)

    """
    convert the continuous rectangle region in the SUNCG dataset to the grid region in the house
    """
    def rescale(self,x1,y1,x2,y2,n_row=None):
        if n_row is None: n_row = self.n_row
        tiny = 1e-9
        tx1 = np.floor((x1 - self.L_lo) / self.L_det * n_row+tiny)
        ty1 = np.floor((y1 - self.L_lo) / self.L_det * n_row+tiny)
        tx2 = np.floor((x2 - self.L_lo) / self.L_det * n_row+tiny)
        ty2 = np.floor((y2 - self.L_lo) / self.L_det * n_row+tiny)
        return int(tx1),int(ty1),int(tx2),int(ty2)

    def to_grid(self, x, y, n_row=None):
        """
        Convert the true-scale coordinate in SUNCG dataset to grid location
        """
        if n_row is None: n_row = self.n_row
        tiny = 1e-9
        tx = np.floor((x - self.L_lo) / self.L_det * n_row + tiny)
        ty = np.floor((y - self.L_lo) / self.L_det * n_row + tiny)
        return int(tx), int(ty)

    def to_coor(self, x, y, shft=False):
        """
        Convert grid location to SUNCG dataset continuous coordinate (the grid center will be returned when shft is True)
        """
        tx, ty = x * self.grid_det + self.L_lo, y * self.grid_det + self.L_lo
        if shft:
            tx += 0.5 * self.grid_det
            ty += 0.5 * self.grid_det
        return tx, ty

    def _check_grid_occupy(self,cx,cy,gx,gy):
        for x in range(gx,gx+2):
            for y in range(gy,gy+2):
                rx, ry = x * self.grid_det + self.L_lo, y * self.grid_det + self.L_lo
                if (rx-cx)**2+(ry-cy)**2<=self.robotRad*self.robotRad:
                    return True
        return False

    """
    suppose the robot stands at continuous coordinate (cx, cy), check whether it will touch any obstacles
    """
    def check_occupy(self, cx, cy): # cx, cy are real coordinates
        radius = self.robotRad
        x1,y1,x2,y2=self.rescale(cx-radius,cy-radius,cx+radius,cy+radius)
        for xx in range(x1,x2+1):
            for yy in range(y1,y2+1):
                if (not self.inside(xx,yy) or self.obsMap[xx,yy] == 1) \
                    and self._check_grid_occupy(cx,cy,xx,yy):
                    return False
        return True

    """
    check if an agent can reach grid location (gx, gy)
    """
    def canMove(self, gx, gy):
        return (self.inside(gx, gy)) and (self.moveMap[gx, gy] > 0)

    """
    check if grid location (gx, gy) is connected to the target room
    """
    def isConnect(self, gx, gy):
        return (self.inside(gx, gy)) and (self.connMap[gx, gy] != -1)

    """
    get the raw shortest distance from grid location (gx, gy) to the target room
    """
    def getDist(self, gx, gy):
        return self.connMap[gx, gy]

    """
    return a scaled shortest distance, which ranges from 0 to 1
    """
    def getScaledDist(self, gx, gy):
        ret = self.connMap[gx, gy]
        if ret < 0:
            return ret
        return ret / self.maxConnDist


    """
    returns all rooms of a given type
    """
    def _getRooms(self, roomTp):
        rooms = [
            r for r in self.all_rooms
            if any([_equal_room_tp(tp, roomTp) for tp in r['roomTypes']])
        ]
        return rooms


    """
    return whether or not a given room type exists in the house
    """
    def hasRoomType(self, roomTp):
        rooms = self._getRooms(roomTp)
        return len(rooms) > 0


    #######################
    # DEBUG functionality #
    #######################
    def _showDebugMap(self, filename=None):
        if self._debugMap is None:
            print('[Warning] <showDebugMap>: Please set DebugInfoOn=True before calling this method!')
        else:
            import matplotlib.pyplot as plt
            import seaborn as sns
            ax = sns.heatmap(self._debugMap[:,::-1])
            if filename is None:
                plt.show()
            else:
                ax.get_figure().savefig(filename)

    def _showObsMap(self):
        import matplotlib.pyplot as plt
        import seaborn as sns
        plt.clf()
        sns.heatmap(self.obsMap[:,::-1])
        plt.show()

    def _showMoveMap(self, visualize=True):
        import matplotlib.pyplot as plt
        import seaborn as sns
        proj = np.array(self.obsMap, dtype=np.float32)
        for x in range(self.n_row+1):
            for y in range(self.n_row+1):
                if self.canMove(x, y):
                    proj[x,y] = 0.5
        if visualize:
            plt.clf()
            ax = sns.heatmap(proj[:,::-1])
            if visualize:
                plt.show()
        return proj

    def _showConnMap(self):
        import matplotlib.pyplot as plt
        import seaborn as sns
        proj = self._showMoveMap(False)
        for x in range(self.n_row+1):
            for y in range(self.n_row+1):
                if self.isConnect(x,y):
                    proj[x,y] = 0.25
        plt.clf()
        sns.heatmap(proj[:,::-1])
        plt.show()
        return proj
