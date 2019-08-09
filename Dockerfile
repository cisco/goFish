FROM golang:1.12.7-stretch
LABEL maintainer="Tomas Rigaux"

ADD setup.sh /
RUN /setup.sh FULL

RUN mkdir -p /goFish
COPY . /goFish

CMD ["/goFish/build.sh"]