FROM golang:1.12.7-stretch AS golang
LABEL maintainer="Tomas Rigaux"

ADD setup.sh /
RUN /setup.sh FULL

RUN mkdir -p /goFish
COPY . /goFish

ENV LD_LIBRARY_PATH="LD_LIBRARY_PATH:/usr/local/lib/"

RUN /goFish/build.sh

FROM golang:1.12.7-stretch
COPY --from=golang /goFish .
COPY --from=golang /usr /usr

EXPOSE 3300

CMD ["./GoFish", "3300"]