from controller import Robot

robot = Robot()
timestep = int(robot.getBasicTimeStep())


left_motor = robot.getDevice('left_motor')
right_motor = robot.getDevice('right_motor')


left_motor.setPosition(float('inf'))
right_motor.setPosition(float('inf'))


left_motor.setVelocity(10.0)
right_motor.setVelocity(10.0)

    

while robot.step(timestep) != -1:
    
    pass