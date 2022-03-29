import os # Operating system library
from glob import glob # Handles file path names
from setuptools import setup

package_name = 'atv_pkg'

# Path of the current directory
cur_directory_path = os.path.abspath(os.path.dirname(__file__))

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        
        #path to the launch file
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
        
        #path to the world file
        (os.path.join('share', package_name, 'worlds/'), glob('./worlds/*')),
        
        #Path to the forest file
        (os.path.join('share', package_name, 'models/forest_world/'), glob('./models/forest_world/*')),
        
        #Path to the atv file
        (os.path.join('share', package_name, 'models/atvkda/'), glob('./models/atvkda/*')),
        
        # Path to the world file (i.e. warehouse + global environment)
        (os.path.join('share', package_name,'models/'), glob('./worlds/*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='cecilnik',
    maintainer_email='cecilnik@stud.ntnu.no',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'atv_node = atv_pkg.atv_node:main'
        ],
    },
)
