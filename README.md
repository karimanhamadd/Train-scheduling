# Railway Track Control System Simulator (C, Multithreading)
This project simulates an automated control system for a railway track using POSIX threads in C. It models the scheduling and coordination of multiple trains (threads) with priorities and directional constraints, demonstrating resource contention and synchronization similar to a real operating system's thread scheduler.
## Project Overview
- Simulates trains departing from East or West stations toward a single main track.
- Trains have attributes like direction, priority (high/low), loading time, and crossing time.
- Only one train can be on the main track at a time.
- Trains compete for access to the track based on a priority and fairness policy.
## Concurrency Design
- Each train is represented as a thread.
- Uses **POSIX threads (`pthread`)**, **mutexes**, and **condition variables** for synchronization.
- Ensures mutual exclusion on the main track using `pthread_mutex_lock`.
- Enforces a set of fairness and starvation-avoidance rules using scheduling logic and condition signaling.
