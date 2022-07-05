base_width=1.168
base_length=2.184
base_height=1.26

wheel_radius=0.347311
wheel_width=0.356

wheel_ygap=-1.02
wheel_zoff=0.425
wheel_xoff=0.75

def wheel_origin(x_reflect,y_reflect):
    origin=[x_reflect*wheel_xoff,y_reflect*(base_width/2+wheel_ygap),-wheel_zoff]
    return origin

wheel_origin(-1,-1) #Back Left Wheel
wheel_origin(-1,1) #Back Right Wheel
wheel_origin(1,1) #Front Right Wheel

print(f"Origin, back right wheel: {wheel_origin(-1,1)}")
print(f"Origin, back left wheel: {wheel_origin(-1,-1)}")
print(f"Origin, front right wheel: {wheel_origin(1,1)}")
print(f"Origin, front left wheel: {wheel_origin(1,-1)}")

def box_inertia(m,w,d,h):
    ixx=(m/12)*(h*h+d*d)
    ixy=0.0
    ixz=0.0
    iyy=(m/12)*(w*w + d*d)
    iyz=0.0
    izz=(m/12) * (w*w + h*h)
    return [f"ixx: {ixx}", f"ixy:{ixy}",f"ixz:{ixz}", f"iyy:{iyy}", f"iyz:{iyz}", f"izz: {izz}"]

print(f"Base link inerta: {box_inertia(30,base_width,base_length,base_height)}")

def cylinder_inerta(m,r,h):
    ixx=(m/12)*(3*r*r + h*h)
    ixy=0.0
    ixz=0.0
    iyy=(m/12)*(3*r*r + h*h)
    iyz=0
    izz=(m/2) * (r*r)
    return [f"ixx: {ixx}", f"ixy:{ixy}",f"ixz:{ixz}", f"iyy:{iyy}", f"iyz:{iyz}", f"izz: {izz}"]

print(f"Wheel inertia: {cylinder_inerta(10,wheel_radius,wheel_width)}")
