import socket
import threading
import time
from tkinter import *
from tkinter import simpledialog, messagebox
import logging # For better debugging

# Setup logging (optional, but good for debugging)
logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(levelname)s] %(message)s')

HOST = 'localhost' # Or your server's IP if it's not on the same machine
PORT = 12345       # Make sure this matches your server's port

# KEY_BINDINGS and KEY_LABELS can remain, we'll just use the entry for 1 player.
KEY_BINDINGS = {
    1: ['Return'], # Default key for single player
    # Other bindings are not used but kept for reference if needed
    2: ['a', 'l'],
    3: ['a', 'l', 'Return'],
    4: ['a', 'h', 'l', 'Return']
}

KEY_LABELS = {
    'a': 'A',
    'h': 'H',
    'l': 'L',
    'Return': 'Enter'
}

class ReactionClient:
    def __init__(self, master):
        self.master = master
        master.title("Reaction Game Client (Single Player)")
        master.geometry("400x350") # Can be smaller for single player
        self.conn = None

        # Single player attributes
        self.player_name = "Player" # Default name
        self.key_binding = KEY_BINDINGS[1][0] # e.g., 'Return'
        self.start_time = None # Single value, not a list
        self.status_label = None # Single label, not a list
        self.header_label = None # For the main game status text

        self.listener_thread = None # To keep a reference

        self.setup_connection_ui()
        master.protocol("WM_DELETE_WINDOW", self.on_closing)


    def setup_connection_ui(self):
        """Sets up UI for entering name and connecting."""
        for widget in self.master.winfo_children():
            widget.destroy()

        self.header_label = Label(self.master, text="Reaction Game Setup", font=("Helvetica", 16, "bold"))
        self.header_label.pack(pady=20)

        # Player Name Input
        name_frame = Frame(self.master)
        name_frame.pack(pady=10)
        Label(name_frame, text="Enter your name:").grid(row=0, column=0, padx=5)
        self.name_entry = Entry(name_frame, width=20)
        self.name_entry.insert(0, self.player_name)
        self.name_entry.grid(row=0, column=1, padx=5)

        # Connect Button
        self.connect_button = Button(self.master, text="Connect to Server", command=self.connect_and_register)
        self.connect_button.pack(pady=20)

        self.bottom_status_label = Label(self.master, text="", fg="blue")
        self.bottom_status_label.pack(side=BOTTOM, fill=X, pady=5)

    def update_bottom_status(self, message, color="black"):
        """Safely updates the bottom status label."""
        logging.info(f"Bottom Status: {message}")
        try:
            self.master.after(0, lambda: self.bottom_status_label.config(text=message, fg=color))
        except Exception as e:
             logging.error(f"Error updating bottom status: {e}")

    def connect_and_register(self):
        """Handles connecting to the server and registering the player."""
        self.player_name = self.name_entry.get().strip()
        if not self.player_name:
            self.player_name = "Player" # Default if empty

        self.connect_button.config(state=DISABLED)
        self.update_bottom_status("Connecting...", "blue")

        player_count = 1 # Hardcoded for single player

        # --- Step 1: Inform server about player count (still useful for some server logic) ---
        try:
            logging.info(f"Sending player count ({player_count}) to {HOST}:{PORT}")
            temp_conn = socket.create_connection((HOST, PORT), timeout=5.0)
            temp_conn.sendall(f"PLAYERS:{player_count}".encode())
            response = temp_conn.recv(1024).decode() # Wait for server ack
            logging.info(f"Server response to PLAYERS: {response}")
            temp_conn.close()
            if not response.startswith("OK"): # Assuming server sends "OK..."
                messagebox.showerror("Setup Error", f"Server rejected player count: {response}")
                self.connect_button.config(state=NORMAL)
                self.update_bottom_status("Setup failed.", "red")
                return
        except Exception as e:
            messagebox.showerror("Connection Error", f"Failed to send player count: {e}")
            logging.error(f"Error sending player count: {e}", exc_info=True)
            self.connect_button.config(state=NORMAL)
            self.update_bottom_status("Connection failed.", "red")
            return

        # --- Step 2: Establish main game connection and send name ---
        try:
            self.conn = socket.create_connection((HOST, PORT), timeout=10.0)
            logging.info(f"Main connection established. Sending name: {self.player_name}")
            self.conn.sendall(f"NAMES:{self.player_name}".encode())
            self.update_bottom_status("Connected. Waiting for game...", "green")

            self.setup_game_ui()

            # Start listening for server messages in a separate thread
            if self.listener_thread and self.listener_thread.is_alive():
                 logging.warning("Listener thread already running. This shouldn't happen.")
            self.listener_thread = threading.Thread(target=self.listen_server, daemon=True)
            self.listener_thread.start()

        except Exception as e:
            messagebox.showerror("Connection Error", f"Could not connect or send name: {str(e)}")
            logging.error(f"Connection error: {e}", exc_info=True)
            if self.conn:
                self.conn.close()
                self.conn = None
            self.connect_button.config(state=NORMAL) # Re-enable button on failure
            self.update_bottom_status("Connection failed.", "red")


    def setup_game_ui(self):
        """Sets up the UI for the single player during the game."""
        for widget in self.master.winfo_children(): # Clear connection UI
            widget.destroy()

        self.header_label = Label(self.master, text=f"Player: {self.player_name}", font=("Helvetica", 16, "bold"))
        self.header_label.pack(pady=10)

        # Single frame for the player
        status_frame = Frame(self.master, bd=2, relief=RIDGE, padx=20, pady=20, bg="white")
        status_frame.pack(pady=20, fill=X, padx=20) # Center it more

        key_text = f"(Press '{KEY_LABELS.get(self.key_binding, self.key_binding)}') "
        self.status_label = Label(status_frame, text=f"Waiting for server... {key_text}", font=("Helvetica", 14), bg="white", height=3)
        self.status_label.pack(pady=10)

        self.bottom_status_label = Label(self.master, text="", fg="blue") # Recreate if destroyed
        self.bottom_status_label.pack(side=BOTTOM, fill=X, pady=5)


    def handle_server_message(self, message):
        """Handles messages from the server and updates GUI accordingly."""
        if message == "START":
            self.header_label.config(text="Get Ready...")
            self.status_label.config(text=f"Prepare to react!\n(Press '{KEY_LABELS.get(self.key_binding, self.key_binding)}')", fg="orange")
            self.start_time = None # Reset start time
            # Unbind key from previous round if any, before GO
            self.master.unbind(f"<{self.key_binding}>")

        elif message == "GO":
            self.header_label.config(text="GO!", fg="red")
            self.start_time = time.time() # Record start time
            self.status_label.config(text=f"GO! Press '{KEY_LABELS.get(self.key_binding, self.key_binding)}' NOW!", fg="red")
            # Bind the key for reaction AFTER "GO" is received
            self.master.bind(f"<{self.key_binding}>", self.record_reaction) # No index needed

        elif message.startswith("ERROR:"):
            error_msg = message.split(":",1)[1]
            messagebox.showerror("Server Error", f"Server Error: {error_msg}")
            self.handle_disconnection()

        elif message == "GAMEOVER":
            self.header_label.config(text="Game Over!", fg="blue")
            self.status_label.config(text="Round finished. Waiting...", fg="black")
            messagebox.showinfo("Game Over", "The round is finished. Check server for leaderboard.")
            # Optionally, automatically go back to connection screen
            self.master.after(2000, self.setup_connection_ui)

        # Handle other messages if your server sends them (e.g., RESTART)

    def listen_server(self):
        """Listens for messages from the server in a background thread."""
        logging.info("Listener thread started.")
        while self.conn:
            try:
                data = self.conn.recv(1024)
                if not data:
                    logging.warning("Server disconnected.")
                    self.master.after(0, self.handle_disconnection) # Ensure GUI updates are on main thread
                    break
                message = data.decode()
                logging.info(f"Received from server: {message}")
                # Use self.master.after to call GUI-updating methods from this thread
                self.master.after(0, self.handle_server_message, message)

            except ConnectionResetError:
                logging.warning("Connection to server was reset.")
                self.master.after(0, self.handle_disconnection)
                break
            except socket.timeout:
                logging.warning("Socket timeout while listening to server.")
                # Decide if this should lead to disconnection
            except OSError as e: # Catches errors like "[Errno 9] Bad file descriptor" if conn is closed
                if self.conn: # If conn was not intentionally closed by on_closing or handle_disconnection
                    logging.error(f"Socket error in listener: {e}")
                    self.master.after(0, self.handle_disconnection)
                else:
                    logging.info("Listener thread stopping as connection is None.")
                break # Exit loop
            except Exception as e:
                logging.error(f"Error listening for server messages: {e}", exc_info=True)
                self.master.after(0, self.handle_disconnection)
                break
        logging.info("Listener thread finished.")

    def record_reaction(self, event=None): # event argument is passed by Tkinter bind
        """Records the reaction time when the player presses their key."""
        if self.start_time is None: # GO signal not yet received or already reacted
            logging.warning("Reaction recorded too early or duplicate.")
            return

        rt = time.time() - self.start_time
        self.status_label.config(text=f"Your Time: {rt:.4f}s", fg="green")
        logging.info(f"Reaction time: {rt:.4f}s for player {self.player_name}")

        try:
            if self.conn:
                self.conn.sendall(f"RT:{self.player_name}:{rt:.4f}".encode())
            else:
                logging.warning("Cannot send RT, no connection.")
        except Exception as e:
            logging.error(f"Error sending reaction time: {e}")
            messagebox.showerror("Network Error", f"Failed to send reaction time: {e}")
            self.handle_disconnection() # Or some other error handling

        self.start_time = None # Prevent multiple submissions for the same "GO"
        # Unbind the key to prevent further reactions until next "GO"
        self.master.unbind(f"<{self.key_binding}>")

    def handle_disconnection(self):
        """Handles cleanup and UI reset when disconnected from server."""
        logging.info("Handling disconnection from server...")
        if self.conn:
            try:
                self.conn.close()
            except Exception as e:
                logging.debug(f"Error closing connection (already closed?): {e}")
            self.conn = None

        # Unbind key, if bound
        try:
            self.master.unbind(f"<{self.key_binding}>")
        except TclError: # If master window is destroyed
            pass
        except Exception as e:
            logging.debug(f"Error unbinding key on disconnect: {e}")


        messagebox.showinfo("Disconnected", "Lost connection to the server. Returning to setup screen.")
        try:
            self.setup_connection_ui() # Reset UI
        except TclError: # If master window is already destroyed during shutdown
            logging.warning("Cannot reset UI, window likely destroyed.")


    def on_closing(self):
        """Called when the Tkinter window is closed by the user."""
        logging.info("Client window closing.")
        if self.conn:
            try:
                # Optionally send a "disconnecting" message to server here
                self.conn.close()
            except Exception as e:
                logging.debug(f"Error closing connection during on_closing: {e}")
        self.conn = None # Important to signal the listener thread to stop
        if self.listener_thread and self.listener_thread.is_alive():
            logging.info("Waiting for listener thread to join...")
            self.listener_thread.join(timeout=1.0) # Wait briefly for thread to exit
        self.master.destroy()

# --- Launch GUI ---
if __name__ == "__main__":
    root = Tk()
    app = ReactionClient(root)
    try:
        root.mainloop()
    except KeyboardInterrupt:
        logging.info("KeyboardInterrupt caught, closing application.")
        app.on_closing()