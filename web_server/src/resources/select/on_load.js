const xhr = new XMLHttpRequest();
xhr.onload = () => {
	if (xhr.status == 200) {
		const data = xhr.response;
		let list = document.getElementById("content-container");
		let films_str = "";

		for (const film of data.films)
			films_str += "<div onclick='window.location.assign(\"/watch?id=" + film.id + "\");'>\n" +
							"<img src='/resources/" + film.poster + "'>" +
							"<div>" +
								"<a>" + film.name + "</a>" +
								"<a class='film-type'>" + ((film.type == "film") ? "Фильм" : "Сериал") + "</a>" +
							"</div>" +
						"</div>";
		list.innerHTML += films_str;
	}
};
xhr.open("GET", "/resources/films.json");
xhr.responseType = "json";
xhr.setRequestHeader("Accept", "application/json");
xhr.send();