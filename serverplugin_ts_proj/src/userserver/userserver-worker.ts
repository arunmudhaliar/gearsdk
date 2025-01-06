import { parentPort } from 'worker_threads';
import { server } from './userserver';
import * as ref from 'ref-napi';

const userserver_instance: server.userserver = new server.userserver();

if (parentPort) {
    console.log = (...args: any[]) => {
        parentPort?.postMessage({ type: 'log', data: args });
    };
}

try {
    userserver_instance.run(ref.NULL);
    parentPort?.postMessage('userserver.run() executed successfully');
} catch (error) {
    parentPort?.postMessage(`Error in userserver.run(): ${error}`);
}
