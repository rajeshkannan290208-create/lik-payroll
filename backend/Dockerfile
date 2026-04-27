FROM ubuntu:22.04

RUN apt-get update && apt-get install -y g++

WORKDIR /app

COPY . .

RUN g++ web_server.cpp ApiServer.cpp Employee.cpp Payroll.cpp -o server

EXPOSE 3000

CMD ["./server"]
