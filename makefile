all: serveur1-theCoolPeople serveur2-theCoolPeople serveur3-theCoolPeople

serveur1-theCoolPeople: bin/serveur1-theCoolPeople.o
	gcc $^ -o $@

bin/serveur1-theCoolPeople.o: src/serveur1-theCoolPeople.c
	gcc -c -Wall $^ -o $@

serveur2-theCoolPeople: bin/serveur2-theCoolPeople.o
	gcc $^ -o $@

bin/serveur2-theCoolPeople.o: src/serveur2-theCoolPeople.c
	gcc -c -Wall $^ -o $@

serveur3-theCoolPeople: bin/serveur3-theCoolPeople.o
	gcc $^ -o $@

bin/serveur3-theCoolPeople.o: src/serveur3-theCoolPeople.c
	gcc -c -Wall $^ -o $@

clean:
	\rm -rf *~ bin/*.o serveur1-theCoolPeople serveur2-theCoolPeople serveur3-theCoolPeople
