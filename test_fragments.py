import socket
import threading
import time

# Configuration de la connexion
HOST = 'localhost'  # Adresse IP de votre serveur
PORT = 8083         # Assurez-vous d'utiliser le port de votre serveur

# Fonction pour envoyer des requêtes fragmentées depuis un client
def send_fragmented_request(client_id, fragments, delay=1):
    # Création de la socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # Connexion au serveur
        s.connect((HOST, PORT))
        print(f"Client {client_id} connected to {HOST}:{PORT}")

        # Envoi des fragments
        for i, fragment in enumerate(fragments):
            print(f"Client {client_id}, sending fragment {i + 1}:\n{fragment.decode()}")
            s.sendall(fragment)
            time.sleep(delay)  # Pause entre l'envoi des fragments

        # Réception de la réponse
        response = s.recv(1024)
        print(f"Client {client_id} response:\n{response.decode()}")

    except Exception as e:
        print(f"Client {client_id} error: {e}")

    finally:
        s.close()
        print(f"Client {client_id} disconnected.")

# Fragments pour les deux clients
client1_fragments = [
    b"GET / HTTP/1.1\r\n",
    b"Host: localhost\r\n",
    b"User-Agent: Client1\r\n\r\n"
]

client2_fragments = [
    b"GET /favicon.ico HTTP/1.1\r\n",
    b"Host: localhost\r\n",
    b"User-Agent: Client2\r\n\r\n"
]

# Threads pour exécuter les deux clients simultanément
thread1 = threading.Thread(target=send_fragmented_request, args=(1, client1_fragments))
thread2 = threading.Thread(target=send_fragmented_request, args=(2, client2_fragments))

# Démarrer les threads
thread1.start()
thread2.start()

# Attendre que les threads terminent
thread1.join()
thread2.join()

print("Test completed.")
