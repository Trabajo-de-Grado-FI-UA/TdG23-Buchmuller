FROM node:16-alpine
RUN mkdir /app
WORKDIR /app
COPY ["package.json", "/app/"]
COPY ./ /app
RUN cd /app && rm -rf node_modules/ && npm install && npx prisma generate
EXPOSE ${PORT}
CMD ["npm", "run", "prod"]