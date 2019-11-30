FROM golang:1.12.7-stretch AS golang
LABEL maintainer="Tomas Rigaux"

ADD setup /
RUN /setup FULL

RUN mkdir -p /goFish
COPY . /goFish

ENV LD_LIBRARY_PATH="LD_LIBRARY_PATH:/usr/local/lib/"

RUN /goFish/build

FROM golang:1.12.7-stretch
COPY --from=golang /goFish /goFish
COPY --from=golang /usr /usr
COPY --from=golang /lib /lib

EXPOSE 3300

CMD ["./goFish/GoFish", "3300"]