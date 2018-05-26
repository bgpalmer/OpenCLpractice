SRCS=glui.cpp
EXEC=prog
LNKS=-lglfw -lGLEW
FRWK=-framework OpenGL -framework OpenCL
all:
	g++ ${SRCS} -o ${EXEC} ${FRWK} ${LNKS} 

practice:
	g++ pOpenCL.cpp -o prac -framework OpenCL -std=c++11