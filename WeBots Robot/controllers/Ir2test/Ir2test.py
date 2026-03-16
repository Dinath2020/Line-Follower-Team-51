from controller import Robot

# 1. Create the Robot instance (Turn on the brain)
robot = Robot()

# 2. Get the timestep (the heartbeat of the robot)
timestep = int(robot.getBasicTimeStep())
# Enable sensors (do this ONCE before the loop)
ds_left = robot.getDevice('ds_left')
ds_right = robot.getDevice('ds_right')
left_motor = robot.getDevice('left_motor')
right_motor = robot.getDevice('right_motor')


left_motor.setPosition(float('inf'))
right_motor.setPosition(float('inf'))

ds_left.enable(timestep)
ds_right.enable(timestep)

# ... inside your while loop ...
while robot.step(timestep) != -1:
    # Read values
    left_motor.setVelocity(10.0)
    right_motor.setVelocity(10.0)
    val_left = ds_left.getValue()
    val_right = ds_right.getValue()
    
    # Print them to the console
    print(f"Left: {val_left} | Right: {val_right}")