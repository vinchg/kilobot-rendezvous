# Kilobot - Rendezvous

Kilobots are a low cost robot designed to test algorithms that require multiple individual units. While their capabilities are limited, they can move using differential drive locomotion, communicate with neighbors up to 7cm away, and measure the distance of neighbors. They serve as a perfect enviornment for testing asynchronous distributed systems.

This project covers the task of rendezvous with a robotic swarm. A practical exmaple of this can be seen in search and rescue drones. Should there be a fault in communicates such as in the case where GPS is lost, drones should then be able to reorient themselves at a target location.
