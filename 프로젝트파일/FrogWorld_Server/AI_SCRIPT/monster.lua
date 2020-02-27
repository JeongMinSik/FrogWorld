isAggro = 0;
isRoaming = 0;
isCombat = 0;
target_id = -1;

myid = 999999;
move_count = 0;

function set_uid(id, agg, ro)
	myid = id;
	isAggro = agg;
	isRoaming = ro;
	isCombat = agg;
end

function set_combat(t_id)
	isCombat = 1;
	target_id = t_id;
end

function get_combat()
	return isCombat;
end

function player_move_notify()
	API_Ai_Start(myid, 100);
end

function move_self()
	
	if (target_id == -1) then
		target_id = API_GetTargetID(myid);
	end
	
	if (target_id > -1) then
		
		API_Ai_Start(myid, 300);

		local my_x = API_GetX(myid);
		local my_y = API_GetY(myid);
		local player_x = API_GetX(target_id);
		local player_y = API_GetY(target_id);
		local distance = math.abs( my_x - player_x ) + math.abs( my_y - player_y );

		if (isRoaming == 1) then

			if (API_Is_Visible(myid, target_id) == 1) then
				if (isCombat == 1) then
					API_FindPath(myid, target_id);
				else
					API_Move_Random(myid);
				end
			else
				API_Move_Random(myid);
				target_id  = -1;
				if (isAggro == 0) then
					isCombat = 0;
				end
			end
		end

		if (distance == 1) then
			if (isCombat == 1) then
				API_Attack(myid, target_id);
			end
		end
	else
		if (isAggro == 0) then
			isCombat = 0;
		end
		API_Set_Actived(myid);
		target_id = -1;
	end
end

  