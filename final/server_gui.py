import socket
import threading
import time
import random
import json
from tkinter import *
import logging # For better debugging
import traceback # For printing full tracebacks

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 12345 # Matching the client's port
RESULTS_FILE = 'reaction_results.json'

# Setup logging
logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(levelname)s] %(message)s')

# --- Global Data (shared between threads, ideally needs better management/locks for complex scenarios) ---
clients = [] # List of (connection, name) tuples
reaction_times = {} # {name: time} for the current round
# persistent_results is loaded once and modified, then saved
# Locks for these would be good in a more complex server, but keeping it minimal for now.
clients_lock = threading.Lock()
reaction_times_lock = threading.Lock()
persistent_results_lock = threading.Lock()


def load_results():
    try:
        with open(RESULTS_FILE, 'r') as f:
            logging.info(f"Loading results from {RESULTS_FILE}")
            return json.load(f)
    except FileNotFoundError:
        logging.warning(f"{RESULTS_FILE} not found. Starting with empty results.")
        return {}
    except json.JSONDecodeError:
        logging.error(f"Error decoding {RESULTS_FILE}. Starting with empty results.")
        return {}

def save_results(results_data):
    with persistent_results_lock:
        try:
            with open(RESULTS_FILE, 'w') as f:
                json.dump(results_data, f, indent=4)
                logging.info(f"Results saved to {RESULTS_FILE}")
        except Exception as e:
            logging.error(f"Failed to save results: {e}")

persistent_results = load_results()

class ServerGUI:
    def __init__(self, master):
        self.master = master
        master.title("Reaction Game Server")

        self.label = Label(master, text="Connected Players:")
        self.label.pack(pady=5)

        self.player_listbox = Listbox(master, width=40, height=5)
        self.player_listbox.pack(pady=5)

        self.leaderboard_label = Label(master, text="Leaderboard (Avg Times):")
        self.leaderboard_label.pack(pady=5)

        self.leaderboard_text = Text(master, height=12, width=40, state=DISABLED)
        self.leaderboard_text.pack(pady=5)

        self.status_label = Label(master, text="Status: Initializing...", fg="blue") # For server status messages
        self.status_label.pack(pady=10)

        # self.ready_clients = [] # This wasn't being used, can be removed or repurposed
        self.required_players = 0 # Number of players expected for the current game

        self.start_server()
        self.update_leaderboard_display() # Display initial leaderboard
        self.set_status("Ready. Waiting for player count.", "blue")

        master.protocol("WM_DELETE_WINDOW", self.on_closing)


    def set_status(self, message, color="black"):
        """Safely updates the status label text from any thread."""
        logging.info(f"Server Status: {message}")
        try:
            self.master.after(0, lambda: self.status_label.config(text=f"Status: {message}", fg=color))
        except Exception as e: # Handle Tkinter errors if master is destroyed
            logging.warning(f"Could not update status label (master destroyed?): {e}")

    def update_player_list_display(self):
        """Updates the Listbox showing connected players. Thread-safe via master.after."""
        def do_update():
            logging.debug("Updating player listbox display.")
            self.player_listbox.delete(0, END)
            with clients_lock: # Access global 'clients' safely
                current_client_names = [name for _, name in clients]
            for name in current_client_names:
                self.player_listbox.insert(END, name)
        try:
            self.master.after(0, do_update)
        except Exception as e:
            logging.warning(f"Could not update player list (master destroyed?): {e}")


    def update_leaderboard_display(self):
        """Calculates and displays the leaderboard. Thread-safe via master.after."""
        def do_update():
            logging.debug("Updating leaderboard display.")
            self.leaderboard_text.config(state=NORMAL)
            self.leaderboard_text.delete(1.0, END)

            avg_times_list = []
            with persistent_results_lock:
                current_persistent_results = dict(persistent_results)

            for name, times in current_persistent_results.items():
                if times: # Ensure there are times to average
                    avg = sum(times) / len(times)
                    avg_times_list.append((avg, name))

            avg_times_list.sort() # Sort by average time (ascending)

            if not avg_times_list:
                self.leaderboard_text.insert(END, "No scores yet.")
            else:
                for i, (avg, name) in enumerate(avg_times_list, 1):
                    self.leaderboard_text.insert(END, f"{i}. {name} (Avg: {avg:.4f}s)\n")
            self.leaderboard_text.config(state=DISABLED)
        try:
            self.master.after(0, do_update)
        except Exception as e:
            logging.warning(f"Could not update leaderboard (master destroyed?): {e}")


    def client_thread(self, conn, addr):
        player_names = []
        game_started_by_this_thread = False

        try:
            logging.info(f"Accepted connection from {addr}")
            header = conn.recv(1024).decode()
            logging.info(f"Received from {addr}: {header[:50]}...")

            if header.startswith("PLAYERS:"):
                try:
                    if self.required_players > 0 and self.required_players != int(header.split(":")[1]):
                        logging.warning(f"Player count already set to {self.required_players}. Ignoring {header} from {addr}")
                        conn.sendall(b"ERROR:Count already set or mismatched")
                    else:
                        count = int(header.split(":")[1])
                        if 1 <= count <= 4:
                            self.required_players = count
                            self.set_status(f"Waiting for {self.required_players} players...", "orange")
                            logging.info(f"[SERVER] Required players set to {self.required_players} by {addr}")
                            conn.sendall(b"OK:Count Set")
                        else:
                             logging.warning(f"Invalid player count {count} from {addr}")
                             conn.sendall(b"ERROR:Invalid player count")
                except Exception as e:
                    logging.error(f"Error setting player count from {addr}: {e}")
                    try:
                        conn.sendall(b"ERROR:Bad player count format")
                    except Exception: pass
                return # Exit thread

            if self.required_players == 0:
                 logging.warning(f"Player count not set. Closing connection from {addr} which sent: {header[:50]}")
                 conn.sendall(b"ERROR:Player count not set by an admin client first.")
                 return

            if header.startswith("NAMES:"):
                names_str = header.split(":", 1)[1]
                player_names = [name.strip() for name in names_str.split(",") if name.strip()]
            else:
                player_names = [header.strip()]
                if not player_names[0]:
                     logging.warning(f"Received empty name from {addr}. Closing.")
                     return

            if len(player_names) != self.required_players:
                 logging.warning(f"Expected {self.required_players} names from {addr}, but got {len(player_names)}: {player_names}. Closing.")
                 conn.sendall(f"ERROR:Expected {self.required_players} names".encode())
                 return

            with clients_lock:
                if len(clients) >= self.required_players and not any(c_conn == conn for c_conn, _ in clients) :
                    logging.warning(f"Game is full. Rejecting names from {addr}")
                    conn.sendall(b"ERROR:Game Full")
                    return

                for name in player_names:
                    clients.append((conn, name))
                    logging.info(f"[SERVER] Registered: {name} from {addr} ({len(clients)}/{self.required_players})")
            self.update_player_list_display()

            # --- Modified Game Start Logic ---
            with clients_lock:
                if len(clients) == self.required_players:
                    game_started_by_this_thread = True
                    self.set_status(f"All {self.required_players} players joined. Game starting in 3 seconds...", "green")
                    logging.info(f"[SERVER] All {self.required_players} players joined. Initial 3s delay commencing.")

            if game_started_by_this_thread:
                # 1. Fixed 3-second delay AFTER all players connect
                time.sleep(3)
                logging.info("[SERVER] Initial 3-second delay finished.")

                # 2. Send START signal
                self.set_status(f"Get ready...", "orange")
                logging.info("[SERVER] Sending START to all clients.")
                with clients_lock:
                    for client_conn, _ in clients:
                        try:
                            client_conn.sendall(b"START")
                        except Exception as e:
                            logging.error(f"Failed to send START to a client: {e}")

                # 3. New random delay (1-5 seconds) before sending GO
                go_delay = random.uniform(1, 5) # MODIFIED DELAY RANGE
                logging.info(f"[SERVER] Waiting {go_delay:.2f}s before sending GO...")
                self.set_status(f"Get Ready... GO in {go_delay:.1f}s!", "orange") # Update status
                time.sleep(go_delay)

                # 4. Send GO signal
                with clients_lock:
                    # Clear reaction times for the new round just before GO
                    with reaction_times_lock:
                        reaction_times.clear()
                    logging.info("[SERVER] Sending GO.")
                    for client_conn, _ in clients:
                        try:
                            client_conn.sendall(b"GO")
                        except Exception as e:
                            logging.error(f"Failed to send GO to a client: {e}")
                self.set_status("GO! Waiting for reactions...", "red")
                # --- End of Modified Game Start Logic ---


                # --- Collect Reaction Times ---
                expected_rt_count = len(player_names)
                received_rt_count = 0
                conn.settimeout(10.0)

                while received_rt_count < expected_rt_count:
                    try:
                        data = conn.recv(1024).decode()
                        if not data:
                            logging.warning(f"Connection closed by {addr} while waiting for RT.")
                            break
                        logging.info(f"RT Data from {addr}: {data}")
                        if data.startswith("RT:"):
                            parts = data.split(":")
                            if len(parts) == 3:
                                _, rt_player_name_str, rt_str = parts
                                rt_player_name = rt_player_name_str.strip()
                                try:
                                    rt = float(rt_str)
                                    if rt_player_name in player_names:
                                        with reaction_times_lock:
                                            if rt_player_name not in reaction_times:
                                                reaction_times[rt_player_name] = rt
                                                logging.info(f"[{rt_player_name}] Reaction time: {rt:.4f}s")
                                                received_rt_count += 1
                                            else:
                                                logging.warning(f"Duplicate RT for {rt_player_name} from {addr}")
                                    else:
                                        logging.warning(f"Received RT for unexpected player {rt_player_name} from {addr}")
                                except ValueError:
                                    logging.error(f"Invalid RT float value from {addr}: {rt_str}")
                            else:
                                logging.warning(f"Malformed RT string from {addr}: {data}")
                    except socket.timeout:
                        logging.warning(f"Timeout waiting for reaction time from {addr}. {received_rt_count}/{expected_rt_count} received.")
                        break
                    except ConnectionResetError:
                        logging.warning(f"Connection reset by {addr} during RT collection.")
                        break
                    except Exception as e:
                        logging.error(f"Error receiving RT from {addr}: {e}")
                        break

                conn.settimeout(None)

                with reaction_times_lock, clients_lock:
                     if len(reaction_times) == self.required_players:
                          logging.info("All reactions received for the game.")
                          with persistent_results_lock:
                                for name, t_val in reaction_times.items():
                                    if name not in persistent_results:
                                        persistent_results[name] = []
                                    persistent_results[name].append(t_val)
                          save_results(persistent_results)
                          self.update_leaderboard_display()
                          self.set_status(f"Game finished. Scores recorded.", "green")
                          for c_conn, _ in clients:
                              try: c_conn.sendall(b"GAMEOVER")
                              except: pass
                     elif game_started_by_this_thread:
                          logging.warning(f"Game incomplete. {len(reaction_times)}/{self.required_players} reactions.")
                          self.set_status(f"Game incomplete. {len(reaction_times)}/{self.required_players} reactions.", "orange")

        except ConnectionResetError:
            logging.warning(f"Connection reset by {addr}")
        except socket.timeout:
            logging.warning(f"Socket timeout with {addr}")
        except Exception as e:
            logging.error(f"Unexpected error in client_thread for {addr}: {e}")
            traceback.print_exc()
        finally:
            logging.info(f"Cleaning up connection for {addr}. Player names handled by this thread: {player_names}")
            player_list_updated = False
            with clients_lock:
                for i in range(len(clients) - 1, -1, -1):
                    client_conn_obj, client_name_in_list = clients[i]
                    if client_conn_obj == conn and client_name_in_list in player_names:
                        clients.pop(i)
                        logging.info(f"[SERVER] Removed '{client_name_in_list}' (from {addr}) from active clients.")
                        player_list_updated = True

            if player_list_updated:
                self.update_player_list_display()

            with clients_lock, reaction_times_lock:
                if not clients and self.required_players > 0 and game_started_by_this_thread :
                    logging.info("All clients disconnected from active game. Resetting for new game.")
                    self.required_players = 0
                    reaction_times.clear()
                    self.set_status("All players disconnected. Ready for new player count.", "blue")
                    self.update_player_list_display()
                    self.update_leaderboard_display()

            try:
                conn.close()
                logging.info(f"Closed connection for {addr}")
            except Exception as e:
                logging.debug(f"Error closing connection for {addr} (already closed?): {e}")


    def start_server(self):
        def accept_clients():
            server_socket = None
            try:
                server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                server_socket.bind((HOST, PORT))
                server_socket.listen(5)
                logging.info(f"[*] Server listening on {HOST}:{PORT}...")
                self.set_status(f"Listening on {HOST}:{PORT}", "blue")

                while True:
                    try:
                        conn, addr = server_socket.accept()
                        client_handler_thread = threading.Thread(target=self.client_thread, args=(conn, addr), daemon=True)
                        client_handler_thread.start()
                    except OSError:
                        logging.info("Server socket has been closed. Stopping accept_clients loop.")
                        break
                    except Exception as e:
                        logging.error(f"Error accepting new connection: {e}")
                        time.sleep(0.1)
            except Exception as e:
                logging.critical(f"Server's main accept socket failed: {e}", exc_info=True)
                self.set_status(f"SERVER FAILED TO START: {e}", "red")
            finally:
                if server_socket:
                    server_socket.close()
                    logging.info("Server accept socket closed.")

        self.accept_thread = threading.Thread(target=accept_clients, daemon=True)
        self.accept_thread.start()
        logging.info("Server accept thread started.")

    def on_closing(self):
        logging.info("Server GUI is closing. Shutting down...")
        save_results(persistent_results)
        self.master.destroy()
        logging.info("Server shutdown complete.")


# --- Launch GUI ---
if __name__ == "__main__":
    root = Tk()
    gui_app = ServerGUI(root)
    try:
        root.mainloop()
    except KeyboardInterrupt:
        logging.info("KeyboardInterrupt received. Closing server...")
        gui_app.on_closing()