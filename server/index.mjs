import fs from 'fs'
import https from 'https'
import express from 'express'

const app = express();
app.use(express.json());
app.use(express.static('./web'));
app.use(express.urlencoded({ extended: true }));

app.listen(7000);
