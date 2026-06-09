function create_seasons_tree(film) {
	if (film.type == "film")
		return null;

	let films_str = "<details><summary><a>" + film.name + "<a/></summary><ul class='custom-list'>\n";
	film.seasons.sort((a, b) => (a.num - b.num));
	for (const season of film.seasons)
	{
		films_str += "<li class='dropdown-li'><details><summary><a>Сезон " + season.num + "</a></summary><ul class='custom-list'>\n";
		season.series.sort((a, b) => (a.num - b.num));
		for (const seria of season.series)
			films_str += "<li><a href='/watch?id=" + film.id + "&season=" + season.num + "&series=" + seria.num + "'>Серия " + seria.num + "</a></li>\n"
		films_str += "</ul></details>\n";
	}
	films_str += "</ul></details>\n";

	return films_str;
}