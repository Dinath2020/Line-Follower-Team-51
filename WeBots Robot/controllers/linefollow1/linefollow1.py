from controller import Robot

robot = Robot()
timestep = int(robot.getBasicTimeStep())

# 1. Setup Motors
left_motor = robot.getDevice('left_motor')
right_motor = robot.getDevice('right_motor')
left_motor.setPosition(float('inf'))
right_motor.setPosition(float('inf'))
left_motor.setVelocity(0.0)
right_motor.setVelocity(0.0)

# 2. Setup Sensors
ds_left = robot.getDevice('ds_left')
ds_right = robot.getDevice('ds_right')
ds_left.enable(timestep)
ds_right.enable(timestep)

# 3. Constants
SPEED = -8.0          # Normal speed
TURN_SPEED = -3   # Slow speed for turning
THRESHOLD = 600      # The middle point between 360 and 880

print("Line Follower Started...")

while robot.step(timestep) != -1:
    # Read Sensors
    left_val = ds_left.getValue()
    right_val = ds_right.getValue()
    
    # Logic: "Bang-Bang" Control
    left_sees_black = left_val > THRESHOLD
    right_sees_black = right_val > THRESHOLD

    if right_sees_black:
        # Case A: Line is on the Left -> Turn LEFT
        # (Stop left wheel, drive right wheel)
        left_motor.setVelocity(-TURN_SPEED) # Reverse slightly for sharper turn
        right_motor.setVelocity(SPEED)
        print("Turning right")
        
    elif left_sees_black:
        # Case B: Line is on the Right -> Turn RIGHT
        # (Drive left wheel, stop right wheel)
        left_motor.setVelocity(SPEED)
        right_motor.setVelocity(-TURN_SPEED)
        print("Turning left")
        
    elif (left_sees_black and right_sees_black):
        # Case B: Line is on the Right -> Turn RIGHT
        # (Drive left wheel, stop right wheel)
        left_motor.setVelocity(SPEED)
        right_motor.setVelocity(SPEED)
        print("forward")
    else:
        # Case C: Both on White -> Go FORWARD
        left_motor.setVelocity(SPEED)
        right_motor.setVelocity(SPEED)
        print("Forward")