/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TRECEIVEFROMMON 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 21
#define PRIORITY_TARENA 21
#define PRIORITY_TBATTERY 19
/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_mutex_create(&mutex_withWD, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_image, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_computePositionMode, NULL)) {
        cerr << "Error mutex compute Position Mode create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_arena, NULL)) {
        cerr << "Error mutex compute Position Mode create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_mutex_create(&mutex_closeCamera, NULL)) {
        cerr << "Error mutex close Camera create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_sem_create(&sem_WD, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_camera, NULL, 0, S_FIFO)) {
        cerr << "Error camera semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_create(&th_loadWD, "th_loadWD", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_battery, "th_battery", 0, PRIORITY_TBATTERY, 0)) {
        cerr << "Error battery task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_camera, "th_camera", 0, PRIORITY_TCAMERA, 0)) {
        cerr << "Error camera task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_arena, "th_arena", 0, PRIORITY_TARENA, 0)) {
        cerr << "Error analysis task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if ((err = rt_queue_create(&q_arena, "q_arena", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if ((err = rt_queue_create(&q_messageToCam, "q_messageToCam", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue arena create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    

    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_start(&th_loadWD, (void(*)(void*)) & Tasks::LoadWD, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_battery, (void(*)(void*)) & Tasks::BatteryTask, this)) {
        cerr << "Error battery task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_camera, (void(*)(void*)) & Tasks::CameraTask, this)) {
        cerr << "Error camera task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_arena, (void(*)(void*)) & Tasks::ArenaTask, this)) {
        cerr << "Error analysis task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    int status;

    // Start server
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };

    // Establish connection with monitor
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);

    // Strat reading received message 
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {

        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;
        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
            monitor.Close(); 
            rt_mutex_release(&mutex_monitor);
            cout << "com monitor close " << endl;
            rt_task_delete(&th_receiveFromMon);
            delete(msgRcv);
            exit(-1);
            
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_mutex_acquire(&mutex_withWD, TM_INFINITE);
            withWD = 0;
            rt_mutex_release(&mutex_withWD);
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITH_WD)) {
            rt_mutex_acquire(&mutex_withWD, TM_INFINITE);
            withWD = 1;
            rt_mutex_release(&mutex_withWD);;
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        } else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)) {
            rt_mutex_acquire(&mutex_closeCamera, TM_INFINITE);
            close_camera = false;
            rt_mutex_release(&mutex_closeCamera);
            
            rt_sem_v(&sem_camera);
        } else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)) {
            rt_mutex_acquire(&mutex_closeCamera, TM_INFINITE);
            close_camera = true;
            rt_mutex_release(&mutex_closeCamera);
            
            rt_sem_p(&sem_camera, TM_INFINITE);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_START)) {
            rt_mutex_acquire(&mutex_computePositionMode, TM_INFINITE);
            computePositionMode = true;
            rt_mutex_release(&mutex_computePositionMode);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_STOP)) {
            rt_mutex_acquire(&mutex_computePositionMode, TM_INFINITE);
            computePositionMode = false;
            rt_mutex_release(&mutex_computePositionMode);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ASK_ARENA) ||
                msgRcv->CompareID(MESSAGE_CAM_ARENA_CONFIRM) ||
                msgRcv->CompareID(MESSAGE_CAM_ARENA_INFIRM)) {
            WriteInQueue(&q_arena, msgRcv);
        }
        delete(msgRcv); // must be deleted manually, no consumer
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {
        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        rt_mutex_acquire(&mutex_withWD, TM_INFINITE);
        int WD = withWD;
        rt_mutex_release(&mutex_withWD);
        if (WD == 0){
            // Start without WD
            cout << "Start robot without watchdog (";
            msgSend = robot.Write(robot.StartWithoutWD());
       } else {
            // Start with WD
            cout << "Start robot with watchdog (";
            msgSend = robot.Write(robot.StartWithWD());
            rt_sem_v(&sem_WD);
        }
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;
        cout << "Start answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

void Tasks::LoadWD(void *arg) {

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    rt_sem_p(&sem_WD, TM_INFINITE);
    
    int WD = 0;
    int cpt = 0;
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 1000000000);
    
    while (1){
        
        rt_task_wait_period(NULL); 
        
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        int rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        
        if (rs == 1) {
            rt_mutex_acquire(&mutex_withWD, TM_INFINITE);
            WD = withWD;
            rt_mutex_release(&mutex_withWD);
            
            if (WD) {
                  cout << "cpt: " << cpt << endl << flush;
                  rt_mutex_acquire(&mutex_robot, TM_INFINITE);
                  Message *msg_reload = robot.Write(robot.ReloadWD());
                  rt_mutex_release(&mutex_robot);
                  
                  if(msg_reload->GetID() == MESSAGE_ANSWER_ACK) {
                    cpt = 0;
                  } else {
                    cpt++;
                  }  
                }
            if (cpt > 3){
                rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
                robotStarted = 0;
                rt_mutex_release(&mutex_robotStarted);
                
                rt_mutex_acquire(&mutex_robot, TM_INFINITE);
                robot.Close();
                rt_mutex_release(&mutex_robot);
                
                rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
                WriteInQueue(&q_messageToMon,  new Message(MESSAGE_ANSWER_COM_ERROR));
                rt_mutex_release(&mutex_monitor);
                
                rt_sem_v(&sem_openComRobot);
            }
        }
    }   
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);

            cout << " move: " << cpMove;

            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            robot.Write(new Message((MessageID) cpMove));
            rt_mutex_release(&mutex_robot);
        }
        cout << endl << flush;
    }
}

/**
 * @brief Thread handling battery of the robot.
 */

void Tasks::BatteryTask(void *arg) {
    Message *levelBat;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 500000000);

    while (1) {
        rt_task_wait_period(NULL);

        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        int rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            cout << "Periodic battery update" << endl;
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            levelBat = robot.Write(robot.GetBattery());
            rt_mutex_release(&mutex_robot);

            rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
            WriteInQueue(&q_messageToMon, levelBat);
            rt_mutex_release(&mutex_monitor);
        }
        cout << endl << flush;
    }
}

void Tasks::CameraTask(void *arg) {
    int rs;
    //Message
    Message *failed_open_cam;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/

    // set the period in a way that we send an image every 100ms
    rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(100000000));
    
    //initialisation

    Camera cam;
    bool copy_of_CPM;
    bool close_cam_order;
    bool arena_known = false;
    std::list<Position> robot_position;
    Position pos;
    
    while(1) {
        
        rt_sem_p(&sem_camera, TM_INFINITE);

        bool status_camera = cam.Open();
        if (status_camera == false) {
            failed_open_cam = new Message(MESSAGE_ANSWER_NACK);
            WriteInQueue(&q_messageToMon, failed_open_cam);
        }
        
        //loop
        while (1) {
            rt_mutex_acquire(&mutex_closeCamera, TM_INFINITE);
            close_cam_order = close_camera;
    
            rt_mutex_release(&mutex_closeCamera);
            if (close_cam_order == true) {
                cam.Close();
                rt_sem_v(&sem_camera);
                break;
            }
            // a tester sans variable intermédiare
            rt_mutex_acquire(&mutex_image, TM_INFINITE);
            image = cam.Grab().Copy();
            
            rt_mutex_acquire(&mutex_arena, TM_INFINITE);
            arena_known = (arena != 0);

            rt_mutex_acquire(&mutex_computePositionMode, TM_INFINITE);
            copy_of_CPM = computePositionMode;
            rt_mutex_release(&mutex_computePositionMode);
            
            if (arena_known == true) {
                image->DrawArena(*arena);
            }
            
            if ((copy_of_CPM == true) and (arena_known == true)) {
                robot_position = image->SearchRobot(*arena);
                if (robot_position.size() == 0) {
                    pos = Position();
                    pos.center=cv::Point2f(-1.0,-1.0);
                } else {
                    pos = *robot_position.begin();
                }
                WriteInQueue(&q_messageToMon, new MessagePosition(MESSAGE_CAM_POSITION, pos));
            }          
                
            WriteInQueue(&q_messageToMon, new MessageImg(MESSAGE_CAM_IMAGE, image));

            rt_mutex_release(&mutex_arena);
            rt_mutex_release(&mutex_image);

            rt_task_wait_period(NULL);
        }

    }
}

void Tasks::ArenaTask(void *arg) {
    //Message
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/

    bool done = false;

    while (!done) {

        Message *msg = ReadInQueue(&q_arena);

        if (!msg->CompareID(MESSAGE_CAM_ASK_ARENA)) {
            cerr << "Bad message for Arena" << endl << flush;
            exit(EXIT_FAILURE);
        }

        //Pause camera
        rt_mutex_acquire(&mutex_image, TM_INFINITE);

        //Copy global image and search arena
        Img* img = image->Copy();
        Arena ar = img->SearchArena();

        if (ar.IsEmpty()) {
            WriteInQueue(&q_messageToMon, new Message(MESSAGE_ANSWER_NACK));
        } else {
            img->DrawArena(ar);

            //Send image to monitor for validation
            WriteInQueue(&q_messageToMon, new MessageImg(MESSAGE_CAM_IMAGE, img));

            //Wait for validation
            msg = ReadInQueue(&q_arena);
            if (msg->CompareID(MESSAGE_CAM_ARENA_CONFIRM)) {
                rt_mutex_acquire(&mutex_arena, TM_INFINITE);
                arena = new Arena(ar);
                rt_mutex_release(&mutex_arena);
                done = true;
            }
        }

        //Resume camera
        rt_mutex_release(&mutex_image);
    }
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}

