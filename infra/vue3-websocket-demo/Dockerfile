FROM node:18-alpine
WORKDIR /app

COPY stress_test/package*.json ./

RUN npm install --legacy-peer-deps

COPY stress_test ./

EXPOSE 3000

CMD ["npm", "run", "dev"]