if [ ! -d "_build" ]; then
	mkdir _build
fi

gfortran -c -o ./_build/MeshParserFortran.o MeshParser.f90 
gcc -c -o ./_build/MeshParserC.o MeshParser.c 
gfortran -o ./_build/MeshParser ./_build/MeshParserC.o ./_build/MeshParserFortran.o
