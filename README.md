# graph_message
Distributed messaging program.

This program sends messages over a distributed network of peers. The network is structed as a connected directed graph where each node is an instance of the program running on a machine with a unique "PEER ID" and a table of other peer id's and thier respective IP addresses.
Every peer in the network can send a message to any other peer in the network regradless of if they have them in thier peer table i.e. "know their IP", this is done by passing the message to all peers in the senders peer table, and then they pass the message on through thier peers eventually reaching the peer that the message was intended for.
Notice the network must be a connected graph for this to work there for two different connected components will be considered two different networks.

## To use
Run `./distmsg ./config` in terminal. you will now enter a configuration creation setup or you will enter the CLI depending on if you have the file `config` created at the path you supplied to the program or not.
Once in the program, you can do the following operations.
- Connect to other peers on the network by typing `connect` and the supplying the PEER ID and IP as prompted, notice this will also send your PEER ID and IP to the peer you connected to, this will add you to thier peer table and them to yours and so you can send messages directly to them without going through the network.
- Discover other peers without knowing thier PEER ID and IP by typing `discover`. This will request from all of the peers in your peer table their entire peer table and merge it with yours while overwriting conflicts. Notice that one can discover the entire network by continously running discover which will essentailly preform a breadth first search on the network bu this is not recommended since it can strain the network with a lot of traffic.
- Send message by typing `send` and supplying the PEER ID and message you want to send. Notice not the program does not confirm the message reached it's target but rather just pushes the message through the network.
- Exit, quit the program by typing `exit`
  

## TODO
- Add documentation
- Separate CLI and networking engine so that other clients can be developt for the engine independantly

## In the future
I will need to add a way for 2 peers to encrypt thier messages so that "middle peers" wont be able to view the message content.
I will also need a way to verify the senders PEER ID so that peers cannot spoof each other's PEER ID.

These features can are quite a challenge and will probably each require a project of thier own. For the sake of simplicity I will leave this project like this so it can serve as a base for more complex distributed networking programs.

