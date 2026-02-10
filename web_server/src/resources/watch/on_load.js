const params = new URLSearchParams(document.location.search);
let id = params.get("id");

let seasons_select = document.getElementById("seasons-select");
let series_select = document.getElementById("series-select");
let film = null;


function update_seasons_select() {
	seasons_select.innerHTML = "";
	for (const season of film.seasons)
		seasons_select.innerHTML += "<option value='" + season.num + "'><a>Сезон " + season.num + "</a></option>";
}

function update_series_select() {
	series_select.innerHTML = "";
	let season = film.seasons.filter(item => (item.num == seasons_select.value))[0];
	for (const seria of season.series)
		series_select.innerHTML += "<option value='" + seria.num + "'><a>Серия " + seria.num + "</a></option>";
}

function update_video_source() {
	let season = film.seasons.filter(item => (item.num == seasons_select.value))[0];
	let seria = season.series.filter(item => (item.num == series_select.value))[0];
	document.getElementById("video-container").setAttribute("src", "/resources/" + film.path + "/" + season.path + "/" + seria.path);
}

if (id == null)
	window.location.assign("/404");
else
{
	const xhr = new XMLHttpRequest();
	xhr.onload = () => {
		const data = xhr.response;
		film = data.films.filter(item => (item.id == id))[0];

		if (film == null)
			window.location.assign("/404");
		else
		{
			document.title = film.name;
			document.getElementById("film-name").innerHTML = film.name;
			document.getElementById("poster").setAttribute("src", "/resources/" + film.poster);
			if (film.type == "film")
			{
				document.getElementById("block-for-serials").remove();
				document.getElementById("video-container").setAttribute("src", "/resources/" + film.path);
			}
			else
			{
				film.seasons.sort((a, b) => (a.num - b.num));
				for (let season of film.seasons)
					season.series.sort((a, b) => (a.num - b.num));
				update_seasons_select();
				update_series_select();
				update_video_source();
			}

		}
	}
	xhr.open("GET", "/resources/films.json");
	xhr.responseType = "json";
	xhr.setRequestHeader("Accept", "application/json");
	xhr.send();
}