
"""
Initiates a manual calibration routine and generates code for applying the determined calibration values.
You need to pass a joystick object and its variable name as a symbol. The latter is required for the code generation.
Can be used in two ways:
1) Simply call to calibrate your joystick, the calibration is always applied automatically.
2) Execute once and copy the generated code into your script.
"""
function joystick_calibrate(joystick::Joystick_ID; joystick_symbol::Symbol = :NONE)::Expr
    
    if !joystick_is_connected(joystick)
        log_info("Trying to connect to the joystick.")
        if joystick_connect(joystick, "")
            log_info("Connected to a joystick.")
        else
            log_soft_error("Failed to connect to the joystick.")
            return Expr()
        end
    end
    
    function find_most_moved_joystick_axis_and_its_max_range()::Tuple{UInt8, Int16, Int16} # axis_idx, min value, max value
        raw = joystick_raw(joystick)
        n_axes = raw.axes_length # we are assuming that the number of axes doesn't abruptly change.
        
        # create two Int16 vectors
        joystick_axes_raw_min = fill(Int16(2^15-1), n_axes)
        joystick_axes_raw_max = fill(Int16(-2^15), n_axes)

        # First find the raw min and max values for all axes
        start_time = time()
        while time() - start_time < 3.0 # collect joystick data for 3 seconds
            raw = joystick_raw(joystick)
            for i in 1:n_axes
                if raw.axes[i] < joystick_axes_raw_min[i]
                    joystick_axes_raw_min[i] = raw.axes[i]
                elseif raw.axes[i] > joystick_axes_raw_max[i]
                    joystick_axes_raw_max[i] = raw.axes[i]
                end
            end
        end

        # Determine the axis, which has been moved the most
        most_moved_axis = 1;
        most_moved_delta = Int(joystick_axes_raw_max[1]) - Int(joystick_axes_raw_min[1])
        for i in 2:n_axes
            axis_moved_delta = Int(joystick_axes_raw_max[i]) - Int(joystick_axes_raw_min[i])
            if axis_moved_delta > most_moved_delta
                most_moved_axis = i
                most_moved_delta = axis_moved_delta
            end
        end

        return (most_moved_axis, joystick_axes_raw_min[most_moved_axis], joystick_axes_raw_max[most_moved_axis])
    end
    
    println("Move THROTTLE up and down, reaching min and max.");
    sleep(0.5) # this delay accounts for human reaction time
    throttle_axis_idx, throttle_raw_value_min, throttle_raw_value_max = find_most_moved_joystick_axis_and_its_max_range()
    println("Detected throttle on axis '", throttle_axis_idx, "' with min = ", throttle_raw_value_min, ", and max = ", throttle_raw_value_max)

    println("Move YAW left and right, reaching min and max.");
    sleep(0.5)
    yaw_axis_idx, yaw_raw_value_min, yaw_raw_value_max = find_most_moved_joystick_axis_and_its_max_range()
    println("Detected yaw on axis '", yaw_axis_idx, "' with min = ", yaw_raw_value_min, ", and max = ", yaw_raw_value_max)

    println("Move PITCH up and down, reaching min and max.");
    sleep(0.5)
    pitch_axis_idx, pitch_raw_value_min, pitch_raw_value_max = find_most_moved_joystick_axis_and_its_max_range()
    println("Detected pitch on axis '", pitch_axis_idx, "' with min = ", pitch_raw_value_min, ", and max = ", pitch_raw_value_max)

    println("Move ROLL left and right, reaching min and max.");
    sleep(0.5)
    roll_axis_idx, roll_raw_value_min, roll_raw_value_max = find_most_moved_joystick_axis_and_its_max_range()
    println("Detected roll on axis '", roll_axis_idx, "' with min = ", roll_raw_value_min, ", and max = ", roll_raw_value_max)

    println("Now CENTER all your sticks.");
    sleep(0.5)

    throttle_raw_center = 0.0
    yaw_raw_center = 0.0
    pitch_raw_center = 0.0
    roll_raw_center = 0.0

    start_time = time()
    while time() - start_time < 4.0 # collect joystick data for 4 seconds
        # We are using an Exponential Moving Average to find the 'center' values.
        α = 0.1
        raw = joystick_raw(joystick)
        throttle_raw_center = α * raw.axes[throttle_axis_idx] + (1 - α) * throttle_raw_center
        yaw_raw_center = α * raw.axes[yaw_axis_idx] + (1 - α) * yaw_raw_center
        pitch_raw_center = α * raw.axes[pitch_axis_idx] + (1 - α) * pitch_raw_center
        roll_raw_center = α * raw.axes[roll_axis_idx] + (1 - α) * roll_raw_center
    end

    # applying mapping and range
    # We apply this now so that checking if axes should be inverted becomes easyser
    joystick_map_axis_idx_to_type(joystick, throttle_axis_idx, JOYSTICK_THROTTLE)
    joystick_set_axis_range(joystick, JOYSTICK_THROTTLE, throttle_raw_value_min, round(Int16, throttle_raw_center), throttle_raw_value_max)
    
    joystick_map_axis_idx_to_type(joystick, yaw_axis_idx, JOYSTICK_YAW)
    joystick_set_axis_range(joystick, JOYSTICK_YAW, yaw_raw_value_min, round(Int16, yaw_raw_center), yaw_raw_value_max)
    
    joystick_map_axis_idx_to_type(joystick, pitch_axis_idx, JOYSTICK_PITCH)
    joystick_set_axis_range(joystick, JOYSTICK_PITCH, pitch_raw_value_min, round(Int16, pitch_raw_center), pitch_raw_value_max)
    
    joystick_map_axis_idx_to_type(joystick, roll_axis_idx, JOYSTICK_ROLL)
    joystick_set_axis_range(joystick, JOYSTICK_ROLL, roll_raw_value_min, round(Int16, roll_raw_center), roll_raw_value_max)

    # Now figuring out if axes should be inverted.
    println("Now move all your stick to the MAX position.")
    sleep(1.5)
    
    invert_throttle = false
    invert_yaw = false
    invert_pitch = false
    invert_roll = false
    
    data = joystick_data(joystick)
    println("throttle: ", data.throttle, " yaw: ", data.yaw, " pitch: ", data.pitch, " roll: ", data.roll);

    print("Inverting axes: ")
    if data.throttle < 0
        invert_throttle = true
        print("'throttle' ")
        joystick_invert_axis(joystick, JOYSTICK_THROTTLE)
    end
    if data.yaw < 0
        invert_yaw = true
        print("'yaw' ")
        joystick_invert_axis(joystick, JOYSTICK_YAW)
    end
    if data.pitch < 0
        invert_pitch = true
        print("'pitch' ")
        joystick_invert_axis(joystick, JOYSTICK_PITCH)
    end
    if data.roll < 0
        invert_roll = true
        print("'roll' ")
        joystick_invert_axis(joystick, JOYSTICK_ROLL)
    end
    
    if !(invert_throttle || invert_yaw || invert_pitch || invert_roll)
        print("NONE")
    end
    
    println("All Done!")

    # returning the calibration code
    return quote
        if(!TinyDronesSim.joystick_is_connected($joystick_symbol))
            log_soft_error("The joystick was not connected.")
        else
            TinyDronesSim.joystick_map_axis_idx_to_type($joystick_symbol, $throttle_axis_idx, TinyDronesSim.JOYSTICK_THROTTLE)
            TinyDronesSim.joystick_set_axis_range($joystick_symbol, TinyDronesSim.JOYSTICK_THROTTLE, $throttle_raw_value_min, $(round(Int16, throttle_raw_center)), $throttle_raw_value_max)
            
            TinyDronesSim.joystick_map_axis_idx_to_type($joystick_symbol, $yaw_axis_idx, TinyDronesSim.JOYSTICK_YAW)
            TinyDronesSim.joystick_set_axis_range($joystick_symbol, TinyDronesSim.JOYSTICK_YAW, $yaw_raw_value_min, $(round(Int16, yaw_raw_center)), $yaw_raw_value_max)
            
            TinyDronesSim.joystick_map_axis_idx_to_type($joystick_symbol, $pitch_axis_idx, TinyDronesSim.JOYSTICK_PITCH)
            TinyDronesSim.joystick_set_axis_range($joystick_symbol, TinyDronesSim.JOYSTICK_PITCH, $pitch_raw_value_min, $(round(Int16, pitch_raw_center)), $pitch_raw_value_max)
            
            TinyDronesSim.joystick_map_axis_idx_to_type($joystick_symbol, $roll_axis_idx, TinyDronesSim.JOYSTICK_ROLL)
            TinyDronesSim.joystick_set_axis_range($joystick_symbol, TinyDronesSim.JOYSTICK_ROLL, $roll_raw_value_min, $(round(Int16, roll_raw_center)), $roll_raw_value_max)
            
            if $invert_throttle TinyDronesSim.joystick_invert_axis($joystick_symbol, TinyDronesSim.JOYSTICK_THROTTLE) end
            if $invert_yaw TinyDronesSim.joystick_invert_axis($joystick_symbol, TinyDronesSim.JOYSTICK_YAW) end
            if $invert_pitch TinyDronesSim.joystick_invert_axis($joystick_symbol, TinyDronesSim.JOYSTICK_PITCH) end
            if $invert_roll TinyDronesSim.joystick_invert_axis($joystick_symbol, TinyDronesSim.JOYSTICK_ROLL) end
        end
    end
end
