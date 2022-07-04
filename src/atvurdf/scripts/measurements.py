base_width=1.168
base_length=2.184
base_height=1.26

wheel_radius=0.347311
wheel_width=0.356

wheel_ygap=-1.02
wheel_zoff=0.425
wheel_xoff=0.75

def measurements(x_reflect,y_reflect):
    origin=[x_reflect*wheel_xoff,y_reflect*(base_width/2+wheel_ygap),-wheel_zoff]
    return origin

measurements(-1,-1) #Back Left Wheel
measurements(-1,1) #Back Right Wheel
measurements(1,1) #Front Right Wheel

print(f"Origin, back right wheel: {measurements(-1,1)}")
print(f"Origin, back left wheel: {measurements(-1,-1)}")
print(f"Origin, front right wheel: {measurements(1,1)}")
print(f"Origin, front left wheel: {measurements(1,-1)}")

