
MAIN = mnist

all:
	gcc mnist.c mnist_file.c neural_network.c -lm -o mnist

clean:
	rm ${MAIN}