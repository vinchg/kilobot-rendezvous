# Kilobot - Rendezvous

![alt text](../media/media/1_1.png?raw=true)
![alt text](../media/media/1_2.png?raw=true)

Kilobots are a low cost robot designed to test algorithms that require multiple individual units. While their capabilities are limited, they can move using differential drive locomotion, communicate with neighbors up to 7cm away, and measure the distance of neighbors. They serve as a perfect enviornment for testing asynchronous distributed systems.

This project covers the task of rendezvous with a robotic swarm. A practical exmaple of this can be seen in search and rescue drones. Should there be a fault in communications such as in the case where GPS is lost, drones could then be able to reorient themselves at a target location.

## Prerequisites
* Follow the instructions here to setup the enviornment: https://github.com/swarm-lab/kilobot_cmake_base

## Algorithm:

### Leader Election
![alt text](../media/media/2_1.png?raw=true)
![alt text](../media/media/2_2.png?raw=true)

Through message flooding, a leader is naturally elected:
1. Initially, leader ID of each Kilobot is set to itself.
2. The largest leader ID of Kilobot neighbors is set as the leader.

### Alpha Clock Synchronization
![alt text](../media/media/3_1.gif?raw=true)

Clock synchronization also takes advantage of message flooding:
1. Clock incrmenets only if all neighboring clocks are on the same cycle.
2. If a neighbor's clock is larger, set self's clock to it.

### Rendezvous
![alt text](../media/media/3_2.png?raw=true)

### Target Acquisition
![alt text](../media/media/3_3.png?raw=true)

1. If the leader is within range, target is the leader.
2. Else, target closest neighbor.

### Movement
![alt text](../media/media/3_4.gif?raw=true)
![alt text](../media/media/3_5.gif?raw=true)

1. If no target, rotate clockwise.
2. Else, move toward target and stop if Kilobot reaches distance threshold.

## The Final Result:
![alt text](../media/media/4.gif?raw=true)
