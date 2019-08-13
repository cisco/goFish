FROM golang:1.12.7-stretch
LABEL maintainer="Tomas Rigaux"

ADD setup.sh /
RUN /setup.sh FULL

RUN mkdir -p /goFish
COPY . /goFish

RUN /goFish/build.sh

CMD ["/goFish/build.sh"]