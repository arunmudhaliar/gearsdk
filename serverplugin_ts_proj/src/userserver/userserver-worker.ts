/*
import { parentPort } from 'worker_threads';
import { server } from './userserver';

const userserver_instance: server.userserver = new server.userserver(null, null);

if (parentPort) {
    console.log = (...args: any[]) => {
        parentPort?.postMessage({ type: 'log', data: args });
    };
}

try {
    userserver_instance.run(null);
    parentPort?.postMessage('userserver.run() executed successfully');
} catch (error) {
    parentPort?.postMessage(`Error in userserver.run(): ${error}`);
}
*/