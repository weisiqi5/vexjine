package vtf_tests.forwardleap.threadtypes;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;

public class RealTimeWaitingThread implements Runnable {
	private int port;
	private long timeout;
	
	// Constructor with no arguments creates a time server on default port.
	public RealTimeWaitingThread() {
		port = 8900;
		this.timeout = 1000;
	}

	public RealTimeWaitingThread(int port, long timeout) {
		this.port = port;
		this.timeout = timeout;
	}

	// Accept connections for current time. Lazy Exception thrown.
	private void acceptConnections(int port) throws Exception {
		// Selector for incoming time requests
		Selector acceptSelector = SelectorProvider.provider().openSelector();

		// Create a new server socket and set to non blocking mode
		ServerSocketChannel ssc = ServerSocketChannel.open();
		ssc.configureBlocking(false);

		// Bind the server socket to the local host and port

		InetAddress lh = InetAddress.getLocalHost();
		InetSocketAddress isa = new InetSocketAddress(lh, port);
		ssc.socket().bind(isa);

		// Register accepts on the server socket with the selector. This
		// step tells the selector that the socket wants to be put on the
		// ready list when accept operations occur, so allowing multiplexed
		// non-blocking I/O to take place.
		SelectionKey acceptKey = ssc.register(acceptSelector, SelectionKey.OP_ACCEPT);

		int keysAdded = 0;

		// Here's where everything happens. The select method will
		// return when any operations registered above have occurred, the
		// thread has been interrupted, etc.
		while (true) {
			keysAdded = acceptSelector.select(timeout);
			System.out.println("interrupted");
			if (keysAdded > 0) {
				// Someone is ready for I/O, get the ready keys
				Set readyKeys = acceptSelector.selectedKeys();
				Iterator i = readyKeys.iterator();

				// Walk through the ready keys collection and process date requests.
				while (i.hasNext()) {
					SelectionKey sk = (SelectionKey)i.next();
					i.remove();
					// The key indexes into the selector so you
					// can retrieve the socket that's ready for I/O
					ServerSocketChannel nextReady = 
						(ServerSocketChannel)sk.channel();
					// Accept the date request and send back the date string
					Socket s = nextReady.accept().socket();
					// Write the current time to the socket
					PrintWriter out = new PrintWriter(s.getOutputStream(), true);
					Date now = new Date();
					out.println(now);
					out.close();
				}
			}	
		}
		
		
	}


	@Override
	public void run() {
		try {
			acceptConnections(port);
		} catch (Exception e) {
			e.printStackTrace();
		}			
	}

}
