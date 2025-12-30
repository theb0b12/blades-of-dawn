# --- Project Settings ---
PROJECT = game
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# ALL CPP IMPLEMENTATION FILES THAT MAKE UP THE PROJECT
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)

# ALL HEADER FILES
H_FILES = $(wildcard $(INC_DIR)/*.h) $(wildcard $(INC_DIR)/*.hpp)

# ALL RESOURCE FILES
REZ_FILES =

# YOUR USERNAME (optional, used for submission archive)
USERNAME = logan_bordewin-allen

# --- Compiler Settings ---
CXX = g++
CPPVERSION = -std=c++17
CXXFLAGS = -Wall -Wextra -g -I$(INC_DIR) $(CPPVERSION)

# --- SFML Libraries ---

LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# --- Derived Variables ---
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
TARGET = $(PROJECT)

# --- Archive Info ---
ARCHIVE_EXTENSION = tar.gz
ZIP_NAME = $(PROJECT)_$(USERNAME).$(ARCHIVE_EXTENSION)
ZIPPER = tar -czf
DEL = rm -f
MKDIR = mkdir -p

# --- Default rule ---
all: $(TARGET)

# --- Link rule ---
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LIBS)

# --- Compile rule ---
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	$(MKDIR) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Run target ---
run: $(TARGET)
	./$(TARGET)

# --- Clean up ---
clean:
	$(DEL) $(OBJECTS)
	$(DEL) $(TARGET)
	rm -rf $(OBJ_DIR)


submission:
	@echo "Creating submission file $(ZIP_NAME) ..."
	@echo "...Zipping source files:   $(SRC_FILES) ..."
	@echo "...Zipping header files:   $(H_FILES) ..."
	@echo "...Zipping resource files: $(REZ_FILES)..."
	@echo "...Zipping Makefile..."
	$(ZIPPER) $(ZIP_NAME) $(SRC_FILES) $(H_FILES) $(REZ_FILES) Makefile
	@echo "...$(ZIP_NAME) done!"


# ============ WINDOWS BUILD (SFML 3.0.2) ============
MINGW_CXX = x86_64-w64-mingw32-g++
SFML_WIN = SFML-Windows/SFML-3.0.2
WIN_EXE = $(PROJECT).exe

# Windows build with DLLs
windows:
	@echo "Building Windows executable with SFML 3.0.2..."
	$(MINGW_CXX) -std=c++17 -Wall -Wextra -I$(INC_DIR) -I$(SFML_WIN)/include \
	$(SRC_FILES) -o $(WIN_EXE) \
	-L$(SFML_WIN)/lib \
	-lsfml-graphics -lsfml-window -lsfml-system \
	-lopengl32 -lwinmm -lgdi32
	@echo "Copying SFML DLLs..."
	cp $(SFML_WIN)/bin/sfml-graphics-3.dll .
	cp $(SFML_WIN)/bin/sfml-window-3.dll .
	cp $(SFML_WIN)/bin/sfml-system-3.dll .
	@echo "Windows build complete: $(WIN_EXE)"

# Windows static build (single .exe, no DLLs)
windows-static:
	@echo "Building static Windows executable with SFML 3.0.2..."
	$(MINGW_CXX) -std=c++17 -Wall -Wextra -DSFML_STATIC \
	-I$(INC_DIR) -I$(SFML_WIN)/include \
	$(SRC_FILES) -o $(WIN_EXE) \
	-L$(SFML_WIN)/lib \
	-static -lsfml-graphics-s -lsfml-window-s -lsfml-system-s \
	-lopengl32 -lwinmm -lgdi32 -lfreetype \
	-static-libgcc -static-libstdc++
	@echo "Static Windows build complete: $(WIN_EXE)"