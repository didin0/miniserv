# Variables
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude 
SRCDIR = src
BUILDDIR = build
TARGET = server

# Sources
SOURCES = $(SRCDIR)/HttpServer.cpp $(SRCDIR)/HttpUtils.cpp $(SRCDIR)/HttpRequest.cpp $(SRCDIR)/main.cpp
OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))

# Règles principales
all: $(TARGET)

re: fclean all

# Construction de l'exécutable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Compilation des fichiers objets
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Nettoyage des fichiers objets
clean:
	rm -rf $(BUILDDIR)

# Nettoyage complet (fichiers objets + exécutable)
fclean: clean
	rm -f $(TARGET)

.PHONY: all re clean fclean
