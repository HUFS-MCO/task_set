FROM gcc:latest

COPY . /app
WORKDIR /app

RUN sed -i 's/\r$//' /app/build_all.sh \
	&& chmod +x /app/build_all.sh \
	&& /app/build_all.sh \
	&& ls -l /app

# Provide a simple shim to select task via args
COPY run.sh /app/run.sh
RUN chmod +x /app/run.sh

ENTRYPOINT ["/app/run.sh"]
CMD ["--help"]
