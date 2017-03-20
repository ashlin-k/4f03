SOURCE = client.c append_proc.c verify_proc.c
APPEND_OBJ = append_proc.o append_svc.o append_xdr.o
VERIFY_OBJ = verify_proc.o verify_svc.o verify_xdr.o
CLIENT_OBJ = client.o append_clnt.o append_xdr.o verify_clnt.o verify_xdr.o
EXE = client server_verify server_append
DEPS = append.h verify.h

CC = gcc
CFLAGS = -lnsl

.c.o:
	gcc -c $(CFLAGS) $<

all: $(EXE) 

client: $(CLIENT_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -fopenmp

server_verify: $(VERIFY_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 

server_append: $(APPEND_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

client.o: $(SOURCE) $(DEPS)
	$(CC) -c $(SOURCE) 

verify_xdr.o: verify_xdr.c verify.h
	$(CC) -c verify_xdr.c 

append_xdr.o: append_xdr.c append.h
	$(CC) -c append_xdr.c 

verify_svc.o: verify_svc.c verify.h
	$(CC) -c verify_svc.c 

append_svc.o: append_svc.c append.h
	$(CC) -c append_svc.c 

verify.h: verify.x
	rpcgen verify.x

append.h: append.x
	rpcgen append.x

clean: 
	@echo deleting $(APPEND_OBJ) $(VERIFY_OBJ) $(CLIENT_OBJ) $(DEPS) $(EXE) append_svc.c append_xdr.c verify_svc.c verify_xdr.c
	rm -f $(APPEND_OBJ) $(VERIFY_OBJ) $(CLIENT_OBJ) $(DEPS) $(EXE) append_svc.c append_xdr.c verify_svc.c verify_xdr.c out.txt

client.c: $(DEPS)
verify_proc.c: verify.h
append_proc.c: append.h