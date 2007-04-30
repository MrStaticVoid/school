package org.thestaticvoid.cmsc484blog;

import javax.servlet.*;
import javax.servlet.http.*;
import java.io.*;

public class PostArticleServlet extends HttpServlet {
	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		Object userData = request.getSession().getAttribute("userData");
		if (userData == null) {
			String url = request.getRequestURL().toString();
			url = url.substring(0, url.lastIndexOf('/'));
			response.sendRedirect(url + "/org.thestaticvoid.cmsc484blog.LoginServlet");
		}

		int aid = -1;
		try {
			aid = Integer.parseInt(request.getParameter("aid"));
		} catch (Exception e) {

		}

		String title = request.getParameter("title");
		String content = request.getParameter("content");

		Error error = new Error("");
		PostArticleFormData formData = new PostArticleFormData((title == null) ? "" : title, (content == null) ? "" : content);

		if (content != null)
			content = content.replaceAll("\n", "<br />");

		try {
			SqliteDb database = SqliteDb.getSingleton();
						
			if (title == null || title.equals("") ||
			    content == null || content.equals(""))
				error = new Error("All fields must be filled in.");
			else if (aid > -1) {
				if (database.getArticle(aid) == null)
					error = new Error("The specified article does not exist.");
				else {
					database.addComment(aid, ((UserData) userData).getUid(), title, content);
					String url = request.getRequestURL().toString();
					url = url.substring(0, url.lastIndexOf('/'));
					response.sendRedirect(url + "/org.thestaticvoid.cmsc484blog.ViewArticlesServlet?aid=" + aid);
				}
			} else {
				database.addArticle(((UserData) userData).getUid(), title, content);
				String url = request.getRequestURL().toString();
				url = url.substring(0, url.lastIndexOf('/'));
				response.sendRedirect(url + "/org.thestaticvoid.cmsc484blog.ViewArticlesServlet");
			}
		} catch (Exception e) {
			request.setAttribute("e", e);
			request.getRequestDispatcher("/WEB-INF/jsp/error.jsp").forward(request, response);
		}

		Utils.doHeader(request, response, "Post");
		
		request.setAttribute("error", error);
		request.setAttribute("formData", formData);
		request.getRequestDispatcher("/WEB-INF/jsp/postform.jsp").include(request, response);

		Utils.doFooter(request, response);
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		doGet(request, response);
	}
}