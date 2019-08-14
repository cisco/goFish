FROM golang:1.12.7-stretch
LABEL maintainer="Tomas Rigaux"

EXPOSE 3300

ADD setup.sh /
RUN /setup.sh FULL

RUN mkdir -p /goFish
COPY . /goFish

CMD ["/goFish/GoFish", "3300"]