FROM alpine:3.8
LABEL maintainer="Tomas Rigaux"

RUN mkdir /goFish
COPY . /goFish

EXPOSE 3300

RUN ./goFish/setup.sh FULL

CMD ["./goFish/GoFish", "3300"]