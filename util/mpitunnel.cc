/**********************************************************************
 * MPI Tunnel
 *
 * Author: Andreas Seekircher (aseek@cs.miami.edu)
 * 
 * Usage: 
 *   mpirun -np NP ./mpitunnel "CMD1" "CMD2" PORT1 ...
 * 
 * NP processes are started by MPI. The process with rank 0 executes
 * the command CMD1, the other processes will execute CMD2. The given
 * ports are tunneled from the client nodes to the server node.
 * A %d in the client command will be substituted by the rank of the
 * client.
 * This program exits, when all socket connections to the server are
 * closed.
 * 
 **/
//----------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <csignal>

#include <vector>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <mpi.h>

#include "../lib/Process/process.hh"

//----------------------------------------------------------------------
// MPI messages
enum MPI_MSG_TYPE {MPI_DATA=1, MPI_CONNECT, MPI_CONNECT_ACK, 
                   MPI_CONNECT_CLOSE, MPI_EXIT};
struct MpiData
{
  int id;
  int length;
};
struct MpiConnect
{
  int rank;
  int remoteId;
  long port;
};
struct MpiConnectAck
{
  int id;
  int remoteId;
};
struct MpiConnectClose
{
  int id;
};
//----------------------------------------------------------------------
// connection between mpi rank and socket
struct Connection
{
  int rank;
  int localId;
  int remoteId;
  int socket;
};
//----------------------------------------------------------------------
// socket that waits for a connection
struct ListenPort
{
  int socket;
  long port;
};
//----------------------------------------------------------------------
// global stuff to manage open ports or connections

int myrank;
std::vector<Connection> connectionsPending;
std::vector<Connection> connections;
std::vector<ListenPort> listeningPorts;
Process spawnedProcess;
int connectionId = 0;

//======================================================================
// socket functions

int socketListen(long port)
{
  //create server socket
  int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("socket() failed");
		return -1;
	}
  char val = 1;
  if (0 > ioctl(s, FIONBIO, &val))
  {
    perror("Problem with ioctl()");
    return -1;
  }    
  struct sockaddr_in addr;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( (unsigned short int) port);
	addr.sin_family = AF_INET;
	if(bind(s, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		perror("bind() failed");
		return -1;
	}
	if(listen(s, 3) == -1)
	{
		perror("listen() failed");
		return -1;
	}
  return s;
}

int socketAccept(int s)
{
  //accept socket connection
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
	int	client = accept(s, (sockaddr*)&addr, &addr_size);
  if(client == -1)
  {
    //perror("accept() failed");
    return -1;
  }
  char val = 1;
	return client;	
}

int socketConnect(long port)
{
  int s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1)
	{
		perror("socket() failed");
		return -1;
	}
	struct sockaddr_in addr;			
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons( (unsigned short int) port);
	addr.sin_family = AF_INET;
	if(connect(s, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		perror("connect() failed");
		return -1;
	}
  return s;
}

int socketSend(int s, char* buffer, int buffersize)
{
  int v = send(s, buffer, buffersize, 0);
  if(v < 0)
    perror("send() failed");
  return v;  
}

int socketReceive(int s, char* buffer, int buffersize)
{
  fd_set myset; 
  struct timeval tv;
  tv.tv_sec = 0;
	tv.tv_usec = 1;
  FD_ZERO(&myset); 
  FD_SET(s, &myset); 
  int readsocks = select(s+1, &myset, NULL, NULL, &tv);
  if (readsocks < 0)
    perror("socketReceive: select() failed");
  if (readsocks == 0)
    return -1;  
  return recv(s, buffer, buffersize, 0);
}

//======================================================================
// MPI communication

void mpiSendConnectionRequest(int s, long port)
{
  //create pending connections
  Connection newConnect;
  newConnect.rank = 0;  //always connect to server
  newConnect.localId = connectionId++;
  newConnect.remoteId = -1;
  newConnect.socket = s;
  connectionsPending.push_back(newConnect);
  //send MPI connection request
  MpiConnect request;
  request.rank = myrank;
  request.remoteId = newConnect.localId;
  request.port = port;
  char type = (char)MPI_CONNECT;
  MPI_Status status;
  MPI_Send(&type, 1, MPI_BYTE, 
           newConnect.rank, 99, MPI_COMM_WORLD);  
  MPI_Send(&request, sizeof(request), MPI_BYTE, 
           newConnect.rank, 99, MPI_COMM_WORLD);
}

void mpiSendData(Connection c, char *buffer, int bufferSize)
{
  MpiData mpiData;
  mpiData.id = c.remoteId;
  mpiData.length = bufferSize;
  char type = (char)MPI_DATA;
  MPI_Status status;
  MPI_Send(&type, 1, MPI_BYTE, 
           c.rank, 99, MPI_COMM_WORLD);  
  MPI_Send(&mpiData, sizeof(mpiData), MPI_BYTE, 
           c.rank, 99, MPI_COMM_WORLD);
  MPI_Send(buffer, mpiData.length, MPI_BYTE, 
           c.rank, 99, MPI_COMM_WORLD);  
}

void mpiCloseConnection(Connection c)
{
  MpiConnectClose mpiClose;
  mpiClose.id = c.remoteId;
  char type = (char)MPI_CONNECT_CLOSE;
  MPI_Status status;
  MPI_Send(&type, 1, MPI_BYTE, 
           c.rank, 99, MPI_COMM_WORLD);  
  MPI_Send(&mpiClose, sizeof(mpiClose), MPI_BYTE, 
           c.rank, 99, MPI_COMM_WORLD);
}

void mpiExit(int rank)
{
  char type = (char)MPI_EXIT;
  MPI_Status status;
  MPI_Send(&type, sizeof(type), MPI_BYTE, 
           rank, 99, MPI_COMM_WORLD);
}

void mpiHandleConnectionRequest(int rank)
{
  //read mpi message
  MpiConnect request;
  MPI_Status status;
  MPI_Recv(&request, sizeof(request), MPI_BYTE, 
           rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  //connect
  int s = socketConnect(request.port);
  if(s == -1)
    return;
  //store connection
  Connection newConnect;
  newConnect.rank = request.rank;
  newConnect.localId = connectionId++;
  newConnect.remoteId = request.remoteId;
  newConnect.socket = s;
  connections.push_back(newConnect);
  //send connection ack
  MpiConnectAck connectAck;
  connectAck.id = newConnect.remoteId;
  connectAck.remoteId = newConnect.localId;
  char type = (char)MPI_CONNECT_ACK;
  MPI_Send(&type, 1, MPI_BYTE, 
           request.rank, 99, MPI_COMM_WORLD);  
  MPI_Send(&connectAck, sizeof(connectAck), MPI_BYTE, 
           request.rank, 99, MPI_COMM_WORLD);
}

void mpiHandleConnectionAck(int rank)
{
  //read mpi message
  MpiConnectAck connectAck;
  MPI_Status status;
  MPI_Recv(&connectAck, sizeof(connectAck), MPI_BYTE, 
           rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  //find pending connection
  std::vector<Connection>::iterator iter = connectionsPending.begin();
  while(iter!=connectionsPending.end() && iter->localId!=connectAck.id)
    iter++;
  if(iter != connectionsPending.end())
  {
    //set remoteId and move to connections
    iter->remoteId = connectAck.remoteId;
    connections.push_back(*iter);
    connectionsPending.erase(iter);
    printf("MPITunnel: Client %d successfully connected.\n", myrank);
  }
}

void mpiHandleConnectionClose(int rank)
{
  //read mpi message
  MpiConnectClose mpiClose;
  MPI_Status status;
  MPI_Recv(&mpiClose, sizeof(mpiClose), MPI_BYTE, 
           rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  //find connection
  std::vector<Connection>::iterator iter = connections.begin();
  while(iter != connections.end() && iter->localId != mpiClose.id)
    iter++;
  if(iter != connections.end())  
  {
    //close
    close(iter->socket);
    connections.erase(iter);
    printf("MPITunnel: Close socket on rank %d\n", myrank);
  }
}

void mpiHandleData(int rank)
{
  //read mpi message
  MpiData mpiData;
  MPI_Status status;
  MPI_Recv(&mpiData, sizeof(mpiData), MPI_BYTE, 
           rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  //read data
  char *buffer = new char[mpiData.length+1];
  MPI_Recv(buffer, mpiData.length, MPI_BYTE, 
           rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  //find connection
  std::vector<Connection>::iterator iter = connections.begin();
  while(iter != connections.end() && iter->localId != mpiData.id)
    iter++;
  if(iter != connections.end())
  {
    //send data through socket
    if(socketSend(iter->socket, buffer, mpiData.length) <= 0)
    {
      printf("MPITunnel: Lost connection on rank %d\n", myrank);
      mpiCloseConnection(*iter); //send close msg
      close(iter->socket);       //close socket
      connections.erase(iter);
    }
  }  
  delete[] buffer;
}

//======================================================================

//avoid SIGPIPE crashes, errors are handled after writing to socket
void brokenpipe_handler(int) {} 

void termination_handler(int)
{
  printf("MPITunnel: terminated. Killing process\n");
  spawnedProcess.kill();
}

int main(int argc, char** argv)
{
  if(argc<4)
  {
    printf("start with:\n mpirun -np NPROCESSES ./mpitunnel ");
    printf("\"SERVER COMMAND\" \"CLIENT COMMAND\" PORT1 PORT2 ...\n");
    exit(0);
  }
  
  signal(SIGPIPE, brokenpipe_handler);
  signal(SIGHUP, termination_handler);
  signal(SIGTERM, termination_handler);

  //init MPI
  int np;
  MPI_Status status;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  //start programs
  if(myrank == 0)
  {
    printf("MPITunnel: Starting server  %s\n", argv[1]);
    spawnedProcess = Process(argv[1]);
    spawnedProcess.spawn();
  }
  else
  {
    //open all ports on clients
    if(myrank > 0)
      for(int i=3; i<argc; i++)
      {
        ListenPort newPort;
        newPort.port = atol(argv[i]);
        newPort.socket = socketListen(newPort.port);
        listeningPorts.push_back(newPort);
      }
    //start client program
    usleep((myrank+1)*2000000);
    printf("MPITunnel: Starting client  %s\n", argv[2]);
    char cmd[1024];
    sprintf(cmd, argv[2], myrank);
    spawnedProcess = Process(cmd);
    spawnedProcess.spawn();
  }

  const int buffersize = 102400;
  char buffer[buffersize];  
  bool stop = false;
  bool wasConnected = false;
  while(!stop)
  {
    if(myrank > 0) //if client
    {
      //accept socket connections
      std::vector<ListenPort>::iterator iter = listeningPorts.begin();
      for(; iter != listeningPorts.end(); iter++)
      {
        int s = socketAccept(iter->socket);
        if(s > 0)
          mpiSendConnectionRequest(s, iter->port);
      }
    }
  
    //read MPI messages
    int flag = true;
    while(flag)
    {
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &flag, &status);
      if(flag)
      {
        char msgType;
        MPI_Recv(&msgType, 1, MPI_BYTE,
              status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if(msgType == MPI_CONNECT)
          mpiHandleConnectionRequest(status.MPI_SOURCE);
        else if(msgType == MPI_CONNECT_ACK)
          mpiHandleConnectionAck(status.MPI_SOURCE);
        else if(msgType == MPI_DATA)
          mpiHandleData(status.MPI_SOURCE);
        else if(msgType == MPI_CONNECT_CLOSE)
          mpiHandleConnectionClose(status.MPI_SOURCE);
        else if(msgType == MPI_EXIT)
        {
          printf("MPITunnel: Stopping rank %d\n", myrank);
          stop = true;
        }
        else
          printf("MpiTunnel: undefined MPI message received!\n");
      }
    }
  
    //go through all connections, check sockets for data
    std::vector<Connection>::iterator iter = connections.begin();
    for(; iter != connections.end(); iter++)
    {
      int received = socketReceive(iter->socket, buffer, buffersize-1);
      if(received == 0)
      {
        printf("MPITunnel: Lost connection on rank %d\n", myrank);
        mpiCloseConnection(*iter); //send close msg
        close(iter->socket);       //close socket
        connections.erase(iter);
        break;
      }        
      else if(received > 0)
        mpiSendData(*iter, buffer, received);  
    }

    //check if all connections have been closed
    if(connections.size() > 0)
      wasConnected = true;
    if(myrank == 0 && connections.size() == 0 && wasConnected)
    {
      stop = true;
      printf("MPITunnel: All connections closed.\n");
    }

    usleep(1000);
  }
    
  //send exit signal to all clients
  if(myrank==0)
    for(int i=1; i<np; i++)
      mpiExit(i);
  //close all socket connections
  std::vector<Connection>::iterator iter = connections.begin();
  for(; iter != connections.end(); iter++)
    close(iter->socket);
  //close listening sockets
  std::vector<ListenPort>::iterator iter2 = listeningPorts.begin();
  for(; iter2 != listeningPorts.end(); iter2++)
    close(iter2->socket);
  
  MPI_Finalize();
  return 0;
}

