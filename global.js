import {
  HttpClient
} from 'aurelia-http-client';
import mqtt from 'mqtt';

const ding = new Audio('/sound/ding.mp3');
const client = new HttpClient();
const listData = {
  teams: [],
  categories: [],
  tasks: []
};
const mapData = {
  teams: {},
  workflows: {},
  status: {}
};
const reducer = (list) => {
  return list.reduce((acc, item) => {
    if (item == null) {
      return acc;
    }
    acc[item.id] = item;
    return acc;
  }, {});
}
const responseToMap = (key, response) => {
  response = JSON.parse(response.response);
  if (response.data[key]) {
    listData[key] = response.data[key];
    mapData[key] = reducer(listData[key]);
  }
}

client.post('http://cn.origami3.com:8080/api/data', {
  query: `{
    status{
      id
      name
      color
      description
    }
  }`
}).then((response) => {
  responseToMap('status', response);
  out.defaultStatus = mapData.status['1b96572b-4793-4264-a3de-704932022d82'];
});

client.post('http://cn.origami3.com:8080/api/data', {
  query: `{
    workflows{
      id
      name
      description
    }
  }`
}).then((response) => {
  responseToMap('workflows', response);
});

client.post('http://cn.origami3.com:8080/api/data', {
  query: `{
    teams {
      id
      name
      description
      categories {
        id
        name
        description
        tasks {
          id
          name
          description
          priority
          code
          ctime
          mtime
          workflow{
            id
            name
          }
          status{
            id
            name
          }
          comments{
            id
            name
            ctime
            mtime
            description
          }
        }
      }
    }
  }`
}).then((response) => {
  responseToMap('teams', response);
  out.selectedTeam = listData.teams[0];
});

let handleChat = (message) => {
  console.log('chat message', message);
}

let handleUpdate = (message) => {
  let team = mapData.teams[message.team];
  switch (message.t) {
    case 'tasks':
      let task = message.d;
      let cat = team.categories.filter((c) => c.id == task.category);
      for (let c of cat) {
        c.tasks.filter((t) => t.id == task.id).map((t) => $.extend(t, task));
      }
      return;
    case 'categories':
      return team.categories.push(message.d);
  }
  console.log('we have an update', message);
}

let comm = mqtt.connect('ws://cn.origami3.com:8080')

comm.on('connect', function() {
  comm.subscribe('/api/update');
  comm.subscribe('/api/chat');
})

comm.on('message', function(topic, message) {
  ding.play();
  switch (topic) {
    case '/api/chat':
      return handleChat(JSON.parse(message.toString('UTF-8')));
    case '/api/update':
      return handleUpdate(JSON.parse(message.toString('UTF-8')));
  }
});




let out = {
  defaultStatus: {}, //FIXME hard coded for the moment. look above for where field is set
  listData: listData,
  mapData: mapData,
  selectedTeam: null,
  categoriesFilter: {},
  updt(type, id, data) {
    return client.post('/api/data', {
      query: 'mutation($input:' + type + 'Input!){' + type + 'Update(id:"' + id + '", input:$input){id}}',
      variables: JSON.stringify({
        input: data
      }),
      operationName: null
    }).then((d) => {
      this.sendUpdate(type, data);
      return d;
    }).catch((err) => {
      console.log(err);
    });
  },
  save(type, data) {
    return client.post('/api/data', {
      query: 'mutation($input:' + type + 'Input!){' + type + 'Create(input:$input){id}}',
      variables: JSON.stringify({
        input: data
      })
    }).then((result) => {
      let res = JSON.parse(result.response);
      if (res.error || res.errors) {
        throw new Error(res);
      }
      data.id = res.data[type + 'Create'].id;
      this.sendUpdate(type, data);
      return data;
    });
  },
  sendUpdate(type, data) {
    comm.publish('/api/update', JSON.stringify({
      t: type,
      d: data,
      team: this.selectedTeam.id
    }));
  },
  sendChat(message) {
    comm.publish('/api/chat', JSON.stringify({
      m: message
    }));
  }
};

export default out;
