# graph_message
Distributed messaging program.

This program sends messages over a distributed network of peers. The network is structed as a connected directed graph where each node is an instance of the program running on a machine with a unique "PEER ID" and a table of other peer id's and thier respective IP addresses.
Every peer in the network can send a message to any other peer in the network regradless of if they have them in thier peer table i.e. "know their IP", this is done by passing the message to all peers in the senders peer table, and then they pass the message on through thier peers eventually reaching the peer that the message was intended for.
Notice the network must be a connected graph for this to work there for two different connected components will be considered two different networks.

## To use
Run `./distmsg ./config` in terminal. you will now enter a configuration creation setup or you will enter the CLI depending on if you have the file `config` created at the path you supplied to the program or not.
Now run `./client 127.0.0.1:<interface_port>` replacing interface port with the one in the config file, the default is 8080. Notice that if interface_port is not present in the config file the program will not be able 
to communicate with any remote clients and will simply operate in headless mode while printing messages meant for it to standard out.
Once you run the client you will be at a prompt where you can enter commands to send to the `distmsg` instance. supported commands are:
- `connect <peer_id> <address>` Which will add the provided peer to your peer table and add your credentials to the peer's peer table.
- `discover` Which will request the peer table from all your neighbors and merge those peer tables with yours giving the network greater connectivity.
- `send <peer_id> <message>` Which will send a message to the provided peer over the distributed peer network.
To exist gracefully without locking any ports type `exit` into the client prompt.

The main purpose of the client written here is to provide a working example of the data communication format necessary to send commands and recieve responses from the program.

## TODO
- Finish documentation
  
## Further development
I will need to add a way for 2 peers to encrypt thier messages so that "middle peers" wont be able to view the message content.
I will also need a way to verify the senders PEER ID so that peers cannot spoof each other's PEER ID.

These features can are quite a challenge and will probably each require a project of thier own. For the sake of simplicity I will leave this project like this so it can serve as a base for more complex distributed networking programs.

