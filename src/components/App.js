import React from "react";
function createTask(props) {
  return <li>{props}</li>;
}
function App() {
  const [input, setInput] = React.useState("");
  const [tasks, setTask] = React.useState([]);
  function changeInput(event) {
    setInput(event.target.value);
  }
  function addTask() {
    setTask(function additem(privious) {
      return [...privious, input];
    });
  }

  return (
    <div className="container">
      <div className="heading">
        <h1>To-Do List</h1>
      </div>
      <div className="form">
        <input type="text" onChange={changeInput} />
        <button onClick={addTask}>
          <span>Add</span>
        </button>
      </div>
      <div>
        <ul>{tasks.map(createTask)}</ul>
      </div>
    </div>
  );
}

export default App;
